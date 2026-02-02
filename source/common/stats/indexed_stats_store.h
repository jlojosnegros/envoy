#pragma once

#include <memory>
#include <string>

#include "envoy/stats/stats_index.h"
#include "envoy/stats/store.h"

#include "source/common/stats/index_matcher_impl.h"

#include "absl/container/flat_hash_map.h"
#include "absl/synchronization/mutex.h"

namespace Envoy {
namespace Stats {

/**
 * IndexedStatsStore provides secondary indices for efficient metric lookup.
 *
 * This class wraps a base Store and maintains secondary indices that allow
 * O(k) iteration over subsets of metrics instead of O(n) over all metrics.
 *
 * Usage pattern:
 *   IndexedStatsStore indexed_store(base_store);
 *   auto& idx = indexed_store.registerGaugeIndex("active_connections",
 *       std::make_unique<PrefixSuffixIndexMatcher>("", ".active_connections"));
 *
 *   // Later, to sum all matching gauges:
 *   uint64_t total = 0;
 *   idx.forEach([&total](Gauge& g) { total += g.value(); return true; });
 *
 * Thread Safety: All methods are thread-safe. Indices can be registered and
 * accessed from multiple threads. Individual index operations are protected
 * by the index's internal mutex.
 */
class IndexedStatsStore {
public:
  /**
   * Creates an IndexedStatsStore wrapping a base store.
   * @param base_store the underlying stats store
   */
  explicit IndexedStatsStore(Store& base_store);

  virtual ~IndexedStatsStore() = default;

  // Non-copyable and non-movable
  IndexedStatsStore(const IndexedStatsStore&) = delete;
  IndexedStatsStore& operator=(const IndexedStatsStore&) = delete;
  IndexedStatsStore(IndexedStatsStore&&) = delete;
  IndexedStatsStore& operator=(IndexedStatsStore&&) = delete;

  /**
   * Returns the underlying base store.
   */
  Store& baseStore() { return base_store_; }
  const Store& baseStore() const { return base_store_; }

  /**
   * Registers a new gauge index.
   *
   * If called before any gauges exist, the index will be populated as gauges
   * are created. If called after gauges exist, use registerGaugeIndexWithExisting()
   * to also index existing gauges.
   *
   * @param name unique name for this index
   * @param matcher the matcher that determines which gauges belong in this index
   * @return reference to the created index
   * @throws EnvoyException if an index with this name already exists
   */
  GaugeIndex& registerGaugeIndex(const std::string& name, IndexMatcherPtr matcher);

  /**
   * Registers a gauge index and populates it with existing matching gauges.
   *
   * This is useful when indices are registered at runtime (e.g., via xDS)
   * after metrics have already been created.
   *
   * @param name unique name for this index
   * @param matcher the matcher that determines which gauges belong in this index
   * @return reference to the created index
   * @throws EnvoyException if an index with this name already exists
   */
  GaugeIndex& registerGaugeIndexWithExisting(const std::string& name, IndexMatcherPtr matcher);

  /**
   * Registers a new counter index.
   *
   * @param name unique name for this index
   * @param matcher the matcher that determines which counters belong in this index
   * @return reference to the created index
   * @throws EnvoyException if an index with this name already exists
   */
  CounterIndex& registerCounterIndex(const std::string& name, IndexMatcherPtr matcher);

  /**
   * Registers a counter index and populates it with existing matching counters.
   *
   * @param name unique name for this index
   * @param matcher the matcher that determines which counters belong in this index
   * @return reference to the created index
   * @throws EnvoyException if an index with this name already exists
   */
  CounterIndex& registerCounterIndexWithExisting(const std::string& name, IndexMatcherPtr matcher);

  /**
   * Gets a gauge index by name.
   * @param name the index name
   * @return pointer to the index, or nullptr if not found
   */
  GaugeIndex* getGaugeIndex(const std::string& name);
  const GaugeIndex* getGaugeIndex(const std::string& name) const;

  /**
   * Gets a counter index by name.
   * @param name the index name
   * @return pointer to the index, or nullptr if not found
   */
  CounterIndex* getCounterIndex(const std::string& name);
  const CounterIndex* getCounterIndex(const std::string& name) const;

  /**
   * Removes a gauge index.
   * @param name the index name
   * @return true if the index was found and removed
   */
  bool removeGaugeIndex(const std::string& name);

  /**
   * Removes a counter index.
   * @param name the index name
   * @return true if the index was found and removed
   */
  bool removeCounterIndex(const std::string& name);

  /**
   * Called when a new gauge is created. Adds the gauge to all matching indices.
   * This should be called by the store implementation when metrics are created.
   * @param gauge the newly created gauge
   */
  void onGaugeCreated(Gauge& gauge);

  /**
   * Called when a new counter is created. Adds the counter to all matching indices.
   * @param counter the newly created counter
   */
  void onCounterCreated(Counter& counter);

  /**
   * Called when a gauge is being deleted. Removes from all indices.
   * @param gauge the gauge being deleted
   */
  void onGaugeDeleted(Gauge& gauge);

  /**
   * Called when a counter is being deleted. Removes from all indices.
   * @param counter the counter being deleted
   */
  void onCounterDeleted(Counter& counter);

  /**
   * Returns the number of gauge indices.
   */
  size_t gaugeIndexCount() const;

  /**
   * Returns the number of counter indices.
   */
  size_t counterIndexCount() const;

  /**
   * Iterates over all gauge index names.
   * @param fn callback function; return false to stop iteration
   */
  template <typename Fn> void forEachGaugeIndex(Fn fn) const {
    absl::MutexLock lock(&mutex_);
    for (const auto& pair : gauge_indices_) {
      if (!fn(pair.first, *pair.second)) {
        return;
      }
    }
  }

  /**
   * Iterates over all counter index names.
   * @param fn callback function; return false to stop iteration
   */
  template <typename Fn> void forEachCounterIndex(Fn fn) const {
    absl::MutexLock lock(&mutex_);
    for (const auto& pair : counter_indices_) {
      if (!fn(pair.first, *pair.second)) {
        return;
      }
    }
  }

private:
  Store& base_store_;
  mutable absl::Mutex mutex_;
  absl::flat_hash_map<std::string, GaugeIndexPtr> gauge_indices_ ABSL_GUARDED_BY(mutex_);
  absl::flat_hash_map<std::string, CounterIndexPtr> counter_indices_ ABSL_GUARDED_BY(mutex_);
};

using IndexedStatsStorePtr = std::unique_ptr<IndexedStatsStore>;
using IndexedStatsStoreSharedPtr = std::shared_ptr<IndexedStatsStore>;

} // namespace Stats
} // namespace Envoy
