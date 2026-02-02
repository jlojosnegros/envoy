#include "source/common/stats/aggregated_stats_index.h"

#include "source/common/stats/index_matcher_impl.h"
#include "source/common/stats/isolated_store_impl.h"

#include "test/test_common/utility.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Stats {
namespace {

class AggregatedStatsIndexTest : public testing::Test {
protected:
  AggregatedStatsIndexTest() : scope_(store_.rootScope()) {}

  IsolatedStoreImpl store_;
  ScopeSharedPtr scope_;
};

// Gauge aggregation tests

TEST_F(AggregatedStatsIndexTest, GaugeSumEmpty) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("test.", "");
  AggregatedGaugeIndex index("test_gauges", std::move(matcher));

  EXPECT_EQ(0, index.sum());
  EXPECT_EQ(0, index.count());
}

TEST_F(AggregatedStatsIndexTest, GaugeSum) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("conn.", "");
  AggregatedGaugeIndex index("conn_gauges", std::move(matcher));

  Gauge& g1 = scope_->gaugeFromString("conn.a", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("conn.b", Gauge::ImportMode::Accumulate);
  Gauge& g3 = scope_->gaugeFromString("conn.c", Gauge::ImportMode::Accumulate);

  g1.set(100);
  g2.set(200);
  g3.set(50);

  index.tryAdd(g1);
  index.tryAdd(g2);
  index.tryAdd(g3);

  EXPECT_EQ(350, index.sum());
  EXPECT_EQ(3, index.count());
}

TEST_F(AggregatedStatsIndexTest, GaugeAverage) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("avg.", "");
  AggregatedGaugeIndex index("avg_gauges", std::move(matcher));

  Gauge& g1 = scope_->gaugeFromString("avg.a", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("avg.b", Gauge::ImportMode::Accumulate);
  Gauge& g3 = scope_->gaugeFromString("avg.c", Gauge::ImportMode::Accumulate);
  Gauge& g4 = scope_->gaugeFromString("avg.d", Gauge::ImportMode::Accumulate);

  g1.set(10);
  g2.set(20);
  g3.set(30);
  g4.set(40);

  index.tryAdd(g1);
  index.tryAdd(g2);
  index.tryAdd(g3);
  index.tryAdd(g4);

  EXPECT_DOUBLE_EQ(25.0, index.average());
}

TEST_F(AggregatedStatsIndexTest, GaugeAverageEmpty) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("empty.", "");
  AggregatedGaugeIndex index("empty_gauges", std::move(matcher));

  EXPECT_DOUBLE_EQ(0.0, index.average());
}

TEST_F(AggregatedStatsIndexTest, GaugeMinMax) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("mm.", "");
  AggregatedGaugeIndex index("mm_gauges", std::move(matcher));

  Gauge& g1 = scope_->gaugeFromString("mm.a", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("mm.b", Gauge::ImportMode::Accumulate);
  Gauge& g3 = scope_->gaugeFromString("mm.c", Gauge::ImportMode::Accumulate);

  g1.set(50);
  g2.set(10);
  g3.set(100);

  index.tryAdd(g1);
  index.tryAdd(g2);
  index.tryAdd(g3);

  EXPECT_EQ(10, index.min());
  EXPECT_EQ(100, index.max());
}

TEST_F(AggregatedStatsIndexTest, GaugeMinMaxEmpty) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("empty.", "");
  AggregatedGaugeIndex index("empty_gauges", std::move(matcher));

  EXPECT_EQ(UINT64_MAX, index.min());
  EXPECT_EQ(0, index.max());
}

TEST_F(AggregatedStatsIndexTest, GaugeComputeStats) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("stats.", "");
  AggregatedGaugeIndex index("stats_gauges", std::move(matcher));

  Gauge& g1 = scope_->gaugeFromString("stats.a", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("stats.b", Gauge::ImportMode::Accumulate);
  Gauge& g3 = scope_->gaugeFromString("stats.c", Gauge::ImportMode::Accumulate);

  g1.set(100);
  g2.set(50);
  g3.set(150);

  index.tryAdd(g1);
  index.tryAdd(g2);
  index.tryAdd(g3);

  uint64_t sum, min_val, max_val;
  size_t count;
  index.computeStats(sum, min_val, max_val, count);

  EXPECT_EQ(300, sum);
  EXPECT_EQ(50, min_val);
  EXPECT_EQ(150, max_val);
  EXPECT_EQ(3, count);
}

TEST_F(AggregatedStatsIndexTest, GaugeComputeStatsEmpty) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("empty.", "");
  AggregatedGaugeIndex index("empty_gauges", std::move(matcher));

  uint64_t sum, min_val, max_val;
  size_t count;
  index.computeStats(sum, min_val, max_val, count);

  EXPECT_EQ(0, sum);
  EXPECT_EQ(0, min_val); // Adjusted for empty case
  EXPECT_EQ(0, max_val);
  EXPECT_EQ(0, count);
}

// Counter aggregation tests

TEST_F(AggregatedStatsIndexTest, CounterSum) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("", ".total");
  AggregatedCounterIndex index("total_counters", std::move(matcher));

  Counter& c1 = scope_->counterFromString("cluster.a.rq.total");
  Counter& c2 = scope_->counterFromString("cluster.b.rq.total");
  Counter& c3 = scope_->counterFromString("cluster.c.rq.total");

  c1.add(1000);
  c2.add(2000);
  c3.add(500);

  index.tryAdd(c1);
  index.tryAdd(c2);
  index.tryAdd(c3);

  EXPECT_EQ(3500, index.sum());
  EXPECT_EQ(3, index.count());
}

// Custom aggregation tests

TEST_F(AggregatedStatsIndexTest, CustomAggregation) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("custom.", "");
  AggregatedGaugeIndex index("custom_gauges", std::move(matcher));

  Gauge& g1 = scope_->gaugeFromString("custom.a", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("custom.b", Gauge::ImportMode::Accumulate);

  g1.set(3);
  g2.set(4);

  index.tryAdd(g1);
  index.tryAdd(g2);

  // Product aggregation
  uint64_t product = index.aggregate<uint64_t>(1, [](uint64_t acc, uint64_t val) {
    return acc * val;
  });
  EXPECT_EQ(12, product);

  // Sum of squares
  uint64_t sum_squares = index.aggregate<uint64_t>(0, [](uint64_t acc, uint64_t val) {
    return acc + (val * val);
  });
  EXPECT_EQ(25, sum_squares); // 9 + 16
}

// Value updates reflect in aggregation

TEST_F(AggregatedStatsIndexTest, ValueUpdatesReflected) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("upd.", "");
  AggregatedGaugeIndex index("upd_gauges", std::move(matcher));

  Gauge& g1 = scope_->gaugeFromString("upd.a", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("upd.b", Gauge::ImportMode::Accumulate);

  g1.set(10);
  g2.set(20);

  index.tryAdd(g1);
  index.tryAdd(g2);

  EXPECT_EQ(30, index.sum());

  // Update values
  g1.set(50);
  g2.add(30); // Now 50

  EXPECT_EQ(100, index.sum());

  // Decrement
  g1.sub(10); // Now 40
  EXPECT_EQ(90, index.sum());
}

// Removal from index

TEST_F(AggregatedStatsIndexTest, RemovalUpdatesAggregation) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("rem.", "");
  AggregatedGaugeIndex index("rem_gauges", std::move(matcher));

  Gauge& g1 = scope_->gaugeFromString("rem.a", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("rem.b", Gauge::ImportMode::Accumulate);
  Gauge& g3 = scope_->gaugeFromString("rem.c", Gauge::ImportMode::Accumulate);

  g1.set(100);
  g2.set(200);
  g3.set(300);

  index.tryAdd(g1);
  index.tryAdd(g2);
  index.tryAdd(g3);

  EXPECT_EQ(600, index.sum());
  EXPECT_EQ(3, index.count());

  index.remove(g2);

  EXPECT_EQ(400, index.sum());
  EXPECT_EQ(2, index.count());
}

// Real-world use case: Active connections monitoring

TEST_F(AggregatedStatsIndexTest, ActiveConnectionsUseCase) {
  auto matcher = std::make_unique<PrefixSuffixIndexMatcher>("", ".active_connections");
  AggregatedGaugeIndex index("active_connections", std::move(matcher));

  // Simulate multiple clusters with active connections
  Gauge& cluster_a = scope_->gaugeFromString("cluster.web.active_connections",
                                              Gauge::ImportMode::Accumulate);
  Gauge& cluster_b = scope_->gaugeFromString("cluster.api.active_connections",
                                              Gauge::ImportMode::Accumulate);
  Gauge& cluster_c = scope_->gaugeFromString("cluster.db.active_connections",
                                              Gauge::ImportMode::Accumulate);
  // This shouldn't match
  Gauge& total_rq = scope_->gaugeFromString("cluster.web.total_requests",
                                             Gauge::ImportMode::Accumulate);

  cluster_a.set(150);
  cluster_b.set(75);
  cluster_c.set(25);
  total_rq.set(10000);

  index.tryAdd(cluster_a);
  index.tryAdd(cluster_b);
  index.tryAdd(cluster_c);
  EXPECT_FALSE(index.tryAdd(total_rq)); // Shouldn't match

  // Resource monitor can now efficiently get total active connections
  uint64_t total_active = index.sum();
  EXPECT_EQ(250, total_active);

  // Simulate connections changing
  cluster_a.add(50);  // +50 to web cluster
  cluster_b.sub(25);  // -25 from api cluster

  total_active = index.sum();
  EXPECT_EQ(275, total_active);

  // Get detailed stats in single pass
  uint64_t sum, min_val, max_val;
  size_t count;
  index.computeStats(sum, min_val, max_val, count);

  EXPECT_EQ(275, sum);
  EXPECT_EQ(25, min_val);   // db cluster
  EXPECT_EQ(200, max_val);  // web cluster after +50
  EXPECT_EQ(3, count);
}

} // namespace
} // namespace Stats
} // namespace Envoy
