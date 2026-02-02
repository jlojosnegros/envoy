#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "envoy/common/pure.h"
#include "envoy/stats/index_matcher.h"
#include "envoy/stats/stats.h"

#include "absl/container/flat_hash_set.h"
#include "absl/synchronization/mutex.h"

namespace Envoy {
namespace Stats {

/**
 * A secondary index for efficient lookup of metrics matching specific criteria.
 *
 * StatsIndex provides O(k) iteration over a subset of k metrics that match
 * an IndexMatcher, instead of O(n) iteration over all n metrics. This is
 * particularly useful for:
 * - Resource monitors that need to sum specific gauges
 * - Admin endpoints that filter metrics by pattern
 * - Stats sinks that only export specific metrics
 *
 * Thread Safety: All methods are thread-safe. The index uses a mutex internally
 * to protect concurrent access. The forEach callback is invoked while holding
 * the lock, so callbacks should be fast and should not call back into the index.
 *
 * @tparam T The metric type (Counter, Gauge, or Histogram)
 */
template <typename T> class StatsIndex {
public:
  /**
   * Creates a new stats index.
   * @param name a unique name for this index (for debugging/admin)
   * @param matcher the matcher that determines which metrics belong in this index
   */
  StatsIndex(std::string name, IndexMatcherPtr matcher)
      : name_(std::move(name)), matcher_(std::move(matcher)) {}

  virtual ~StatsIndex() = default;

  // Non-copyable and non-movable
  StatsIndex(const StatsIndex&) = delete;
  StatsIndex& operator=(const StatsIndex&) = delete;
  StatsIndex(StatsIndex&&) = delete;
  StatsIndex& operator=(StatsIndex&&) = delete;

  /**
   * Returns the name of this index.
   */
  const std::string& name() const { return name_; }

  /**
   * Returns the matcher used by this index.
   */
  const IndexMatcher& matcher() const { return *matcher_; }

  /**
   * Attempts to add a metric to the index if it matches.
   * @param metric the metric to potentially add
   * @return true if the metric was added (matched), false otherwise
   */
  bool tryAdd(T& metric) {
    if (!matcher_->matches(metric.name())) {
      return false;
    }
    absl::MutexLock lock(&mutex_);
    metrics_set_.insert(&metric);
    return true;
  }

  /**
   * Attempts to add a metric using StatName for potentially faster matching.
   * @param metric the metric to potentially add
   * @param symbol_table the symbol table for StatName decoding
   * @return true if the metric was added (matched), false otherwise
   */
  bool tryAddWithStatName(T& metric, const SymbolTable& symbol_table) {
    if (!matcher_->matchesStatName(metric.statName(), symbol_table)) {
      return false;
    }
    absl::MutexLock lock(&mutex_);
    metrics_set_.insert(&metric);
    return true;
  }

  /**
   * Removes a metric from the index.
   * @param metric the metric to remove
   */
  void remove(T& metric) {
    absl::MutexLock lock(&mutex_);
    metrics_set_.erase(&metric);
  }

  /**
   * Returns a snapshot of all metrics in the index.
   * Note: This creates a copy; for iteration prefer forEach().
   * @return vector of pointers to metrics in the index
   */
  std::vector<T*> metrics() const {
    absl::MutexLock lock(&mutex_);
    return std::vector<T*>(metrics_set_.begin(), metrics_set_.end());
  }

  /**
   * Returns the number of metrics in the index.
   */
  size_t size() const {
    absl::MutexLock lock(&mutex_);
    return metrics_set_.size();
  }

  /**
   * Returns true if the index is empty.
   */
  bool empty() const {
    absl::MutexLock lock(&mutex_);
    return metrics_set_.empty();
  }

  /**
   * Iterates over all metrics in the index, calling fn for each.
   * The callback should be fast as it's called with the mutex held.
   *
   * @param fn the callback function; return false to stop iteration early
   * @return true if iteration completed, false if stopped early by callback
   */
  template <typename Fn> bool forEach(Fn fn) const {
    absl::MutexLock lock(&mutex_);
    for (T* metric : metrics_set_) {
      if (!fn(*metric)) {
        return false;
      }
    }
    return true;
  }

  /**
   * Clears all metrics from the index.
   */
  void clear() {
    absl::MutexLock lock(&mutex_);
    metrics_set_.clear();
  }

protected:
  // Allow derived classes access to the metrics set
  absl::flat_hash_set<T*>& metricsSetLockHeld() ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_) {
    return metrics_set_;
  }

  const absl::flat_hash_set<T*>& metricsSetLockHeld() const ABSL_EXCLUSIVE_LOCKS_REQUIRED(mutex_) {
    return metrics_set_;
  }

  absl::Mutex& mutex() const ABSL_LOCK_RETURNED(mutex_) { return mutex_; }

private:
  const std::string name_;
  const IndexMatcherPtr matcher_;
  mutable absl::Mutex mutex_;
  absl::flat_hash_set<T*> metrics_set_ ABSL_GUARDED_BY(mutex_);
};

// Type aliases for common index types
using GaugeIndex = StatsIndex<Gauge>;
using CounterIndex = StatsIndex<Counter>;
using GaugeIndexPtr = std::unique_ptr<GaugeIndex>;
using CounterIndexPtr = std::unique_ptr<CounterIndex>;

} // namespace Stats
} // namespace Envoy
