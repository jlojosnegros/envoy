#include "source/common/stats/indexed_stats_store.h"

#include "source/common/stats/isolated_store_impl.h"

#include "test/test_common/utility.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Stats {
namespace {

class IndexedStatsStoreTest : public testing::Test {
protected:
  IndexedStatsStoreTest() : indexed_store_(store_), scope_(store_.rootScope()) {}

  IsolatedStoreImpl store_;
  IndexedStatsStore indexed_store_;
  ScopeSharedPtr scope_;
};

// Gauge index tests

TEST_F(IndexedStatsStoreTest, RegisterGaugeIndex) {
  auto& index = indexed_store_.registerGaugeIndex(
      "cluster_gauges", std::make_unique<PrefixSuffixIndexMatcher>("cluster.", ""));

  EXPECT_EQ("cluster_gauges", index.name());
  EXPECT_EQ(1, indexed_store_.gaugeIndexCount());
  EXPECT_EQ(&index, indexed_store_.getGaugeIndex("cluster_gauges"));
}

TEST_F(IndexedStatsStoreTest, RegisterGaugeIndexDuplicatePanics) {
  indexed_store_.registerGaugeIndex("test",
                                     std::make_unique<PrefixSuffixIndexMatcher>("a.", ""));

  EXPECT_DEATH(indexed_store_.registerGaugeIndex(
                   "test", std::make_unique<PrefixSuffixIndexMatcher>("b.", "")),
               "already exists");
}

TEST_F(IndexedStatsStoreTest, GetGaugeIndexNotFound) {
  EXPECT_EQ(nullptr, indexed_store_.getGaugeIndex("nonexistent"));
}

TEST_F(IndexedStatsStoreTest, RemoveGaugeIndex) {
  indexed_store_.registerGaugeIndex("to_remove",
                                     std::make_unique<PrefixSuffixIndexMatcher>("x.", ""));
  EXPECT_EQ(1, indexed_store_.gaugeIndexCount());

  EXPECT_TRUE(indexed_store_.removeGaugeIndex("to_remove"));
  EXPECT_EQ(0, indexed_store_.gaugeIndexCount());
  EXPECT_EQ(nullptr, indexed_store_.getGaugeIndex("to_remove"));

  EXPECT_FALSE(indexed_store_.removeGaugeIndex("to_remove")); // Already removed
}

TEST_F(IndexedStatsStoreTest, OnGaugeCreatedAddsToMatchingIndices) {
  auto& cluster_idx = indexed_store_.registerGaugeIndex(
      "cluster", std::make_unique<PrefixSuffixIndexMatcher>("cluster.", ""));
  auto& listener_idx = indexed_store_.registerGaugeIndex(
      "listener", std::make_unique<PrefixSuffixIndexMatcher>("listener.", ""));

  Gauge& g1 = scope_->gaugeFromString("cluster.foo.active", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("listener.bar.active", Gauge::ImportMode::Accumulate);
  Gauge& g3 = scope_->gaugeFromString("http.baz.active", Gauge::ImportMode::Accumulate);

  indexed_store_.onGaugeCreated(g1);
  indexed_store_.onGaugeCreated(g2);
  indexed_store_.onGaugeCreated(g3);

  EXPECT_EQ(1, cluster_idx.size());
  EXPECT_EQ(1, listener_idx.size());

  // Verify correct assignment
  bool found_g1 = false;
  cluster_idx.forEach([&](Gauge& g) {
    if (&g == &g1) {
      found_g1 = true;
    }
    return true;
  });
  EXPECT_TRUE(found_g1);
}

TEST_F(IndexedStatsStoreTest, OnGaugeDeletedRemovesFromIndices) {
  auto& index = indexed_store_.registerGaugeIndex(
      "test", std::make_unique<PrefixSuffixIndexMatcher>("test.", ""));

  Gauge& g1 = scope_->gaugeFromString("test.gauge1", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("test.gauge2", Gauge::ImportMode::Accumulate);

  indexed_store_.onGaugeCreated(g1);
  indexed_store_.onGaugeCreated(g2);
  EXPECT_EQ(2, index.size());

  indexed_store_.onGaugeDeleted(g1);
  EXPECT_EQ(1, index.size());

  indexed_store_.onGaugeDeleted(g2);
  EXPECT_TRUE(index.empty());
}

TEST_F(IndexedStatsStoreTest, RegisterGaugeIndexWithExisting) {
  // Create gauges first
  Gauge& g1 = scope_->gaugeFromString("existing.gauge1", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("existing.gauge2", Gauge::ImportMode::Accumulate);
  Gauge& g3 = scope_->gaugeFromString("other.gauge", Gauge::ImportMode::Accumulate);

  // Register index after gauges exist
  auto& index = indexed_store_.registerGaugeIndexWithExisting(
      "existing", std::make_unique<PrefixSuffixIndexMatcher>("existing.", ""));

  EXPECT_EQ(2, index.size());

  // Verify the correct gauges are indexed
  std::vector<Gauge*> indexed;
  index.forEach([&](Gauge& g) {
    indexed.push_back(&g);
    return true;
  });
  EXPECT_EQ(2, indexed.size());
  EXPECT_TRUE(std::find(indexed.begin(), indexed.end(), &g1) != indexed.end());
  EXPECT_TRUE(std::find(indexed.begin(), indexed.end(), &g2) != indexed.end());
  EXPECT_TRUE(std::find(indexed.begin(), indexed.end(), &g3) == indexed.end());
}

// Counter index tests

TEST_F(IndexedStatsStoreTest, RegisterCounterIndex) {
  auto& index = indexed_store_.registerCounterIndex(
      "total_counters", std::make_unique<PrefixSuffixIndexMatcher>("", ".total"));

  EXPECT_EQ("total_counters", index.name());
  EXPECT_EQ(1, indexed_store_.counterIndexCount());
  EXPECT_EQ(&index, indexed_store_.getCounterIndex("total_counters"));
}

TEST_F(IndexedStatsStoreTest, OnCounterCreatedAddsToMatchingIndices) {
  auto& index = indexed_store_.registerCounterIndex(
      "rq_total", std::make_unique<PrefixSuffixIndexMatcher>("", ".rq_total"));

  Counter& c1 = scope_->counterFromString("cluster.foo.rq_total");
  Counter& c2 = scope_->counterFromString("cluster.bar.rq_total");
  Counter& c3 = scope_->counterFromString("cluster.baz.rq_error");

  indexed_store_.onCounterCreated(c1);
  indexed_store_.onCounterCreated(c2);
  indexed_store_.onCounterCreated(c3);

  EXPECT_EQ(2, index.size());
}

TEST_F(IndexedStatsStoreTest, RegisterCounterIndexWithExisting) {
  Counter& c1 = scope_->counterFromString("pre.counter1");
  Counter& c2 = scope_->counterFromString("pre.counter2");

  auto& index = indexed_store_.registerCounterIndexWithExisting(
      "pre_counters", std::make_unique<PrefixSuffixIndexMatcher>("pre.", ""));

  EXPECT_EQ(2, index.size());
}

// Base store access

TEST_F(IndexedStatsStoreTest, BaseStoreAccess) {
  EXPECT_EQ(&store_, &indexed_store_.baseStore());

  const IndexedStatsStore& const_indexed = indexed_store_;
  EXPECT_EQ(&store_, &const_indexed.baseStore());
}

// Iteration tests

TEST_F(IndexedStatsStoreTest, ForEachGaugeIndex) {
  indexed_store_.registerGaugeIndex("idx1", std::make_unique<PrefixSuffixIndexMatcher>("a.", ""));
  indexed_store_.registerGaugeIndex("idx2", std::make_unique<PrefixSuffixIndexMatcher>("b.", ""));
  indexed_store_.registerGaugeIndex("idx3", std::make_unique<PrefixSuffixIndexMatcher>("c.", ""));

  std::vector<std::string> names;
  indexed_store_.forEachGaugeIndex([&](const std::string& name, const GaugeIndex&) {
    names.push_back(name);
    return true;
  });

  EXPECT_EQ(3, names.size());
  EXPECT_TRUE(std::find(names.begin(), names.end(), "idx1") != names.end());
  EXPECT_TRUE(std::find(names.begin(), names.end(), "idx2") != names.end());
  EXPECT_TRUE(std::find(names.begin(), names.end(), "idx3") != names.end());
}

TEST_F(IndexedStatsStoreTest, ForEachGaugeIndexEarlyTermination) {
  indexed_store_.registerGaugeIndex("idx1", std::make_unique<PrefixSuffixIndexMatcher>("a.", ""));
  indexed_store_.registerGaugeIndex("idx2", std::make_unique<PrefixSuffixIndexMatcher>("b.", ""));
  indexed_store_.registerGaugeIndex("idx3", std::make_unique<PrefixSuffixIndexMatcher>("c.", ""));

  int count = 0;
  indexed_store_.forEachGaugeIndex([&](const std::string&, const GaugeIndex&) {
    ++count;
    return count < 2; // Stop after 2
  });

  EXPECT_EQ(2, count);
}

// Aggregation use case test

TEST_F(IndexedStatsStoreTest, SumActiveConnectionsUseCase) {
  // Register an index for active connection gauges
  auto& index = indexed_store_.registerGaugeIndex(
      "active_connections", std::make_unique<PrefixSuffixIndexMatcher>("", ".active_connections"));

  // Create some connection gauges
  Gauge& g1 = scope_->gaugeFromString("cluster.a.active_connections", Gauge::ImportMode::Accumulate);
  Gauge& g2 = scope_->gaugeFromString("cluster.b.active_connections", Gauge::ImportMode::Accumulate);
  Gauge& g3 = scope_->gaugeFromString("cluster.c.active_connections", Gauge::ImportMode::Accumulate);
  Gauge& g4 = scope_->gaugeFromString("cluster.a.total_connections", Gauge::ImportMode::Accumulate);

  indexed_store_.onGaugeCreated(g1);
  indexed_store_.onGaugeCreated(g2);
  indexed_store_.onGaugeCreated(g3);
  indexed_store_.onGaugeCreated(g4);

  // Set values
  g1.set(100);
  g2.set(200);
  g3.set(50);
  g4.set(1000); // This shouldn't be included

  // Sum using the index - O(k) where k=3 instead of O(n)
  uint64_t total = 0;
  index.forEach([&total](Gauge& g) {
    total += g.value();
    return true;
  });

  EXPECT_EQ(350, total);
  EXPECT_EQ(3, index.size());
}

} // namespace
} // namespace Stats
} // namespace Envoy
