#include "source/common/stats/stats_index_factory.h"

#include "source/common/stats/isolated_store_impl.h"

#include "test/test_common/utility.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Stats {
namespace {

class StatsIndexFactoryTest : public testing::Test {
protected:
  StatsIndexFactoryTest() : indexed_store_(store_), scope_(store_.rootScope()) {}

  IsolatedStoreImpl store_;
  IndexedStatsStore indexed_store_;
  ScopeSharedPtr scope_;
};

// Matcher creation tests

TEST_F(StatsIndexFactoryTest, CreatePrefixSuffixMatcher) {
  envoy::config::metrics::v3::PrefixSuffixMatcher config;
  config.set_prefix("cluster.");
  config.set_suffix(".upstream_rq");

  auto matcher = StatsIndexFactory::createPrefixSuffixMatcher(config);

  EXPECT_TRUE(matcher->matches("cluster.foo.upstream_rq"));
  EXPECT_TRUE(matcher->matches("cluster.bar.baz.upstream_rq"));
  EXPECT_FALSE(matcher->matches("listener.foo.upstream_rq"));
  EXPECT_FALSE(matcher->matches("cluster.foo.downstream_rq"));
}

TEST_F(StatsIndexFactoryTest, CreatePrefixOnlyMatcher) {
  envoy::config::metrics::v3::PrefixSuffixMatcher config;
  config.set_prefix("http.");

  auto matcher = StatsIndexFactory::createPrefixSuffixMatcher(config);

  EXPECT_TRUE(matcher->matches("http.downstream_rq"));
  EXPECT_TRUE(matcher->matches("http.anything"));
  EXPECT_FALSE(matcher->matches("cluster.http.something"));
}

TEST_F(StatsIndexFactoryTest, CreateSuffixOnlyMatcher) {
  envoy::config::metrics::v3::PrefixSuffixMatcher config;
  config.set_suffix(".active");

  auto matcher = StatsIndexFactory::createPrefixSuffixMatcher(config);

  EXPECT_TRUE(matcher->matches("cluster.foo.active"));
  EXPECT_TRUE(matcher->matches("listener.bar.active"));
  EXPECT_FALSE(matcher->matches("cluster.foo.total"));
}

TEST_F(StatsIndexFactoryTest, CreateStringMatcherWithPrefix) {
  envoy::type::matcher::v3::StringMatcher config;
  config.set_prefix("cluster.");

  auto matcher = StatsIndexFactory::createStringMatcher(config);

  EXPECT_TRUE(matcher->matches("cluster.foo"));
  EXPECT_FALSE(matcher->matches("listener.bar"));
}

TEST_F(StatsIndexFactoryTest, CreateStringMatcherWithSuffix) {
  envoy::type::matcher::v3::StringMatcher config;
  config.set_suffix(".total");

  auto matcher = StatsIndexFactory::createStringMatcher(config);

  EXPECT_TRUE(matcher->matches("http.rq.total"));
  EXPECT_FALSE(matcher->matches("http.rq.count"));
}

TEST_F(StatsIndexFactoryTest, CreateStringMatcherWithRegex) {
  envoy::type::matcher::v3::StringMatcher config;
  config.mutable_safe_regex()->set_regex("^cluster\\.[^.]+\\.upstream_rq$");

  auto matcher = StatsIndexFactory::createStringMatcher(config);

  EXPECT_TRUE(matcher->matches("cluster.foo.upstream_rq"));
  EXPECT_FALSE(matcher->matches("cluster.foo.bar.upstream_rq"));
}

// Index creation from config tests

TEST_F(StatsIndexFactoryTest, CreateGaugeIndexFromConfig) {
  envoy::config::metrics::v3::StatsIndexConfig config;
  config.set_name("test_gauge_index");
  config.set_metric_type(envoy::config::metrics::v3::StatsIndexConfig::GAUGE);
  config.mutable_prefix_suffix()->set_prefix("test.");

  auto matcher = StatsIndexFactory::createMatcher(config);
  auto& index = indexed_store_.registerGaugeIndex(config.name(), std::move(matcher));

  EXPECT_EQ("test_gauge_index", index.name());
}

TEST_F(StatsIndexFactoryTest, CreateCounterIndexFromConfig) {
  envoy::config::metrics::v3::StatsIndexConfig config;
  config.set_name("test_counter_index");
  config.set_metric_type(envoy::config::metrics::v3::StatsIndexConfig::COUNTER);
  config.mutable_prefix_suffix()->set_suffix(".total");

  auto matcher = StatsIndexFactory::createMatcher(config);
  auto& index = indexed_store_.registerCounterIndex(config.name(), std::move(matcher));

  EXPECT_EQ("test_counter_index", index.name());
}

// Full config-to-indices flow

TEST_F(StatsIndexFactoryTest, CreateIndicesFromConfig) {
  envoy::config::metrics::v3::StatsIndicesConfig config;

  auto* gauge_index = config.add_indices();
  gauge_index->set_name("active_connections");
  gauge_index->set_metric_type(envoy::config::metrics::v3::StatsIndexConfig::GAUGE);
  gauge_index->mutable_prefix_suffix()->set_suffix(".active_connections");

  auto* counter_index = config.add_indices();
  counter_index->set_name("upstream_rq");
  counter_index->set_metric_type(envoy::config::metrics::v3::StatsIndexConfig::COUNTER);
  counter_index->mutable_prefix_suffix()->set_prefix("cluster.");
  counter_index->mutable_prefix_suffix()->set_suffix(".upstream_rq");

  IndexedStatsStore new_store(store_);
  StatsIndexFactory::createIndicesFromConfig(new_store, config);

  EXPECT_EQ(1, new_store.gaugeIndexCount());
  EXPECT_EQ(1, new_store.counterIndexCount());
  EXPECT_NE(nullptr, new_store.getGaugeIndex("active_connections"));
  EXPECT_NE(nullptr, new_store.getCounterIndex("upstream_rq"));
}

TEST_F(StatsIndexFactoryTest, CreateIndicesFromConfigWithExisting) {
  // Create some metrics first
  Gauge& g1 = scope_->gaugeFromString("test.active_connections", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("other.active_connections", Gauge::ImportMode::Accumulate);
  g1.set(100);
  g2.set(200);

  envoy::config::metrics::v3::StatsIndicesConfig config;
  auto* index_config = config.add_indices();
  index_config->set_name("active_conn");
  index_config->set_metric_type(envoy::config::metrics::v3::StatsIndexConfig::GAUGE);
  index_config->mutable_prefix_suffix()->set_suffix(".active_connections");

  IndexedStatsStore new_store(store_);
  StatsIndexFactory::createIndicesFromConfigWithExisting(new_store, config);

  auto* index = new_store.getGaugeIndex("active_conn");
  ASSERT_NE(nullptr, index);
  EXPECT_EQ(2, index->size());

  // Verify values
  uint64_t sum = 0;
  index->forEach([&sum](Gauge& g) {
    sum += g.value();
    return true;
  });
  EXPECT_EQ(300, sum);
}

// YAML parsing simulation test

TEST_F(StatsIndexFactoryTest, YamlConfigParsing) {
  const std::string yaml = R"EOF(
indices:
- name: "cluster_gauges"
  metric_type: GAUGE
  prefix_suffix:
    prefix: "cluster."
- name: "total_counters"
  metric_type: COUNTER
  prefix_suffix:
    suffix: ".total"
)EOF";

  envoy::config::metrics::v3::StatsIndicesConfig config;
  TestUtility::loadFromYaml(yaml, config);

  EXPECT_EQ(2, config.indices_size());
  EXPECT_EQ("cluster_gauges", config.indices(0).name());
  EXPECT_EQ(envoy::config::metrics::v3::StatsIndexConfig::GAUGE, config.indices(0).metric_type());
  EXPECT_EQ("cluster.", config.indices(0).prefix_suffix().prefix());
  EXPECT_EQ("total_counters", config.indices(1).name());
  EXPECT_EQ(envoy::config::metrics::v3::StatsIndexConfig::COUNTER, config.indices(1).metric_type());
  EXPECT_EQ(".total", config.indices(1).prefix_suffix().suffix());
}

// Edge cases

TEST_F(StatsIndexFactoryTest, EmptyConfig) {
  envoy::config::metrics::v3::StatsIndicesConfig config;

  IndexedStatsStore new_store(store_);
  StatsIndexFactory::createIndicesFromConfig(new_store, config);

  EXPECT_EQ(0, new_store.gaugeIndexCount());
  EXPECT_EQ(0, new_store.counterIndexCount());
}

} // namespace
} // namespace Stats
} // namespace Envoy
