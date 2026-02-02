#include "envoy/stats/stats_index.h"

#include "source/common/stats/index_matcher_impl.h"
#include "source/common/stats/isolated_store_impl.h"

#include "test/test_common/utility.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Stats {
namespace {

class StatsIndexTest : public testing::Test {
protected:
  StatsIndexTest() : scope_(store_.rootScope()) {}

  IsolatedStoreImpl store_;
  ScopeSharedPtr scope_;
};

// GaugeIndex tests

TEST_F(StatsIndexTest, GaugeIndexAddAndRemove) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("cluster.", "");
  GaugeIndex index("cluster_gauges", std::move(matcher));

  Gauge& g1 = scope_->gaugeFromString("cluster.foo.active", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("cluster.bar.active", Gauge::ImportMode::Accumulate);
  Gauge& g3 = scope_->gaugeFromString("http.downstream.active", Gauge::ImportMode::Accumulate);

  EXPECT_TRUE(index.tryAdd(g1));
  EXPECT_TRUE(index.tryAdd(g2));
  EXPECT_FALSE(index.tryAdd(g3)); // Doesn't match prefix

  EXPECT_EQ(2, index.size());
  EXPECT_FALSE(index.empty());

  index.remove(g1);
  EXPECT_EQ(1, index.size());

  index.remove(g2);
  EXPECT_TRUE(index.empty());
}

TEST_F(StatsIndexTest, GaugeIndexForEach) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("test.", "");
  GaugeIndex index("test_gauges", std::move(matcher));

  Gauge& g1 = scope_->gaugeFromString("test.gauge1", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("test.gauge2", Gauge::ImportMode::Accumulate);
  Gauge& g3 = scope_->gaugeFromString("test.gauge3", Gauge::ImportMode::Accumulate);

  g1.set(10);
  g2.set(20);
  g3.set(30);

  index.tryAdd(g1);
  index.tryAdd(g2);
  index.tryAdd(g3);

  // Sum all values
  uint64_t sum = 0;
  index.forEach([&sum](Gauge& g) {
    sum += g.value();
    return true;
  });
  EXPECT_EQ(60, sum);

  // Early termination
  int count = 0;
  bool completed = index.forEach([&count](Gauge&) {
    ++count;
    return count < 2; // Stop after 2
  });
  EXPECT_FALSE(completed);
  EXPECT_EQ(2, count);
}

TEST_F(StatsIndexTest, GaugeIndexMetricsSnapshot) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("snap.", "");
  GaugeIndex index("snap_gauges", std::move(matcher));

  Gauge& g1 = scope_->gaugeFromString("snap.a", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("snap.b", Gauge::ImportMode::Accumulate);

  index.tryAdd(g1);
  index.tryAdd(g2);

  auto metrics = index.metrics();
  EXPECT_EQ(2, metrics.size());

  // Verify the snapshot contains the right pointers
  EXPECT_TRUE(std::find(metrics.begin(), metrics.end(), &g1) != metrics.end());
  EXPECT_TRUE(std::find(metrics.begin(), metrics.end(), &g2) != metrics.end());
}

TEST_F(StatsIndexTest, GaugeIndexClear) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("clear.", "");
  GaugeIndex index("clear_gauges", std::move(matcher));

  Gauge& g1 = scope_->gaugeFromString("clear.a", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("clear.b", Gauge::ImportMode::Accumulate);

  index.tryAdd(g1);
  index.tryAdd(g2);
  EXPECT_EQ(2, index.size());

  index.clear();
  EXPECT_TRUE(index.empty());
  EXPECT_EQ(0, index.size());
}

// CounterIndex tests

TEST_F(StatsIndexTest, CounterIndexBasic) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("", ".total");
  CounterIndex index("total_counters", std::move(matcher));

  Counter& c1 = scope_->counterFromString("http.rq.total");
  Counter& c2 = scope_->counterFromString("http.rs.total");
  Counter& c3 = scope_->counterFromString("http.rq.error");

  EXPECT_TRUE(index.tryAdd(c1));
  EXPECT_TRUE(index.tryAdd(c2));
  EXPECT_FALSE(index.tryAdd(c3)); // Doesn't match suffix

  EXPECT_EQ(2, index.size());

  c1.add(100);
  c2.add(200);

  uint64_t sum = 0;
  index.forEach([&sum](Counter& c) {
    sum += c.value();
    return true;
  });
  EXPECT_EQ(300, sum);
}

// Index properties tests

TEST_F(StatsIndexTest, IndexName) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("", "");
  GaugeIndex index("my_index", std::move(matcher));

  EXPECT_EQ("my_index", index.name());
}

TEST_F(StatsIndexTest, IndexMatcher) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("prefix.", ".suffix");
  GaugeIndex index("test", std::move(matcher));

  EXPECT_EQ("prefix='prefix.' AND suffix='.suffix'", index.matcher().describe());
}

// Duplicate add/remove tests

TEST_F(StatsIndexTest, DuplicateAdd) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("dup.", "");
  GaugeIndex index("dup_gauges", std::move(matcher));

  Gauge& g1 = scope_->gaugeFromString("dup.gauge", Gauge::ImportMode::Accumulate);

  EXPECT_TRUE(index.tryAdd(g1));
  EXPECT_TRUE(index.tryAdd(g1)); // Adding again should succeed (hash set handles it)
  EXPECT_EQ(1, index.size());    // But size should still be 1
}

TEST_F(StatsIndexTest, RemoveNonExistent) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("rm.", "");
  GaugeIndex index("rm_gauges", std::move(matcher));

  Gauge& g1 = scope_->gaugeFromString("rm.gauge", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("rm.other", Gauge::ImportMode::Accumulate);

  index.tryAdd(g1);
  EXPECT_EQ(1, index.size());

  index.remove(g2); // Remove something not in the index
  EXPECT_EQ(1, index.size()); // Should be unchanged
}

// Empty index tests

TEST_F(StatsIndexTest, EmptyIndex) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("empty.", "");
  GaugeIndex index("empty_gauges", std::move(matcher));

  EXPECT_TRUE(index.empty());
  EXPECT_EQ(0, index.size());

  auto metrics = index.metrics();
  EXPECT_TRUE(metrics.empty());

  bool called = false;
  bool completed = index.forEach([&called](Gauge&) {
    called = true;
    return true;
  });
  EXPECT_TRUE(completed);
  EXPECT_FALSE(called);
}

// Regex matcher integration test

TEST_F(StatsIndexTest, RegexMatcherIntegration) {
  auto matcher = std::make_unique<RegexIndexMatcher>("^cluster\\.[^.]+\\.upstream_rq$");
  GaugeIndex index("upstream_rq", std::move(matcher));

  Gauge& g1 = scope_->gaugeFromString("cluster.foo.upstream_rq", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("cluster.bar.upstream_rq", Gauge::ImportMode::Accumulate);
  Gauge& g3 =
      scope_->gaugeFromString("cluster.baz.qux.upstream_rq", Gauge::ImportMode::Accumulate);
  Gauge& g4 = scope_->gaugeFromString("listener.foo.upstream_rq", Gauge::ImportMode::Accumulate);

  EXPECT_TRUE(index.tryAdd(g1));
  EXPECT_TRUE(index.tryAdd(g2));
  EXPECT_FALSE(index.tryAdd(g3)); // Too many segments
  EXPECT_FALSE(index.tryAdd(g4)); // Wrong prefix

  EXPECT_EQ(2, index.size());
}

// Or matcher integration test

TEST_F(StatsIndexTest, OrMatcherIntegration) {
  std::vector<IndexMatcherPtr> matchers;
  matchers.push_back(std::make_unique<PrefixSuffixIndexMatcher>("cluster.", ""));
  matchers.push_back(std::make_unique<PrefixSuffixIndexMatcher>("listener.", ""));
  auto or_matcher = std::make_unique<OrIndexMatcher>(std::move(matchers));

  GaugeIndex index("cluster_or_listener", std::move(or_matcher));

  Gauge& g1 = scope_->gaugeFromString("cluster.foo", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("listener.bar", Gauge::ImportMode::Accumulate);
  Gauge& g3 = scope_->gaugeFromString("http.baz", Gauge::ImportMode::Accumulate);

  EXPECT_TRUE(index.tryAdd(g1));
  EXPECT_TRUE(index.tryAdd(g2));
  EXPECT_FALSE(index.tryAdd(g3));

  EXPECT_EQ(2, index.size());
}

} // namespace
} // namespace Stats
} // namespace Envoy
