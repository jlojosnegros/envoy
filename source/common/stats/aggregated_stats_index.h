#pragma once

#include <atomic>
#include <functional>

#include "envoy/stats/stats_index.h"

namespace Envoy {
namespace Stats {

/**
 * An extended StatsIndex that provides pre-computed aggregated values.
 *
 * AggregatedStatsIndex extends StatsIndex with efficient aggregation:
 * - sum(): Returns the sum of all metric values in O(k) time
 * - count(): Returns the number of metrics in O(1) time
 * - average(): Returns the average value in O(k) time
 * - min()/max(): Returns min/max values in O(k) time
 *
 * Current Implementation:
 *   Aggregation is computed via iteration over the k metrics in the index.
 *   This is O(k) where k << n (total metrics), providing significant
 *   improvement over O(n) iteration over all metrics.
 *
 * Future Enhancement:
 *   If MetricAggregationObserver support is added to core Gauge/Counter
 *   interfaces, this class can be enhanced to maintain running totals
 *   for O(1) sum() operations using the notifyIncrement/notifyDecrement
 *   callbacks.
 *
 * Thread Safety: All methods are thread-safe, inheriting from StatsIndex.
 *
 * @tparam T The metric type (Counter or Gauge)
 */
template <typename T> class AggregatedStatsIndex : public StatsIndex<T> {
public:
  using StatsIndex<T>::StatsIndex;

  /**
   * Returns the sum of all metric values in the index.
   * Time complexity: O(k) where k = number of metrics in index.
   */
  uint64_t sum() const {
    uint64_t total = 0;
    this->forEach([&total](const T& metric) {
      total += metric.value();
      return true;
    });
    return total;
  }

  /**
   * Returns the number of metrics in the index.
   * Time complexity: O(1).
   */
  size_t count() const { return this->size(); }

  /**
   * Returns the average value of metrics in the index.
   * Returns 0 if the index is empty.
   * Time complexity: O(k) where k = number of metrics in index.
   */
  double average() const {
    size_t n = this->size();
    if (n == 0) {
      return 0.0;
    }
    return static_cast<double>(sum()) / static_cast<double>(n);
  }

  /**
   * Returns the minimum value among metrics in the index.
   * Returns UINT64_MAX if the index is empty.
   * Time complexity: O(k) where k = number of metrics in index.
   */
  uint64_t min() const {
    uint64_t result = UINT64_MAX;
    this->forEach([&result](const T& metric) {
      uint64_t val = metric.value();
      if (val < result) {
        result = val;
      }
      return true;
    });
    return result;
  }

  /**
   * Returns the maximum value among metrics in the index.
   * Returns 0 if the index is empty.
   * Time complexity: O(k) where k = number of metrics in index.
   */
  uint64_t max() const {
    uint64_t result = 0;
    this->forEach([&result](const T& metric) {
      uint64_t val = metric.value();
      if (val > result) {
        result = val;
      }
      return true;
    });
    return result;
  }

  /**
   * Applies a custom aggregation function to all metrics.
   *
   * @param initial the initial accumulator value
   * @param fn the aggregation function: (accumulator, metric_value) -> new_accumulator
   * @return the final accumulated value
   */
  template <typename AccT, typename Fn> AccT aggregate(AccT initial, Fn fn) const {
    AccT result = initial;
    this->forEach([&result, &fn](const T& metric) {
      result = fn(result, metric.value());
      return true;
    });
    return result;
  }

  /**
   * Computes multiple aggregations in a single pass.
   * More efficient than calling sum(), min(), max() separately.
   *
   * @param out_sum output for sum
   * @param out_min output for minimum
   * @param out_max output for maximum
   * @param out_count output for count
   */
  void computeStats(uint64_t& out_sum, uint64_t& out_min, uint64_t& out_max,
                    size_t& out_count) const {
    out_sum = 0;
    out_min = UINT64_MAX;
    out_max = 0;
    out_count = 0;

    this->forEach([&](const T& metric) {
      uint64_t val = metric.value();
      out_sum += val;
      if (val < out_min) {
        out_min = val;
      }
      if (val > out_max) {
        out_max = val;
      }
      ++out_count;
      return true;
    });

    // Handle empty case
    if (out_count == 0) {
      out_min = 0;
    }
  }
};

// Type aliases
using AggregatedGaugeIndex = AggregatedStatsIndex<Gauge>;
using AggregatedCounterIndex = AggregatedStatsIndex<Counter>;
using AggregatedGaugeIndexPtr = std::unique_ptr<AggregatedGaugeIndex>;
using AggregatedCounterIndexPtr = std::unique_ptr<AggregatedCounterIndex>;

} // namespace Stats
} // namespace Envoy
