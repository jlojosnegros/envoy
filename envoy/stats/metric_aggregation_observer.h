#pragma once

#include <cstdint>

#include "envoy/common/pure.h"

namespace Envoy {
namespace Stats {

/**
 * Observer interface for metric value changes.
 *
 * Implementations of this interface can register with metrics to receive
 * notifications when values change. This enables O(1) aggregation by
 * maintaining running totals that update incrementally rather than
 * requiring O(n) iteration to compute sums.
 *
 * Thread Safety: Implementations must be thread-safe as notifications
 * may come from multiple threads concurrently.
 */
class MetricAggregationObserver {
public:
  virtual ~MetricAggregationObserver() = default;

  /**
   * Called when a counter or gauge is incremented.
   * @param delta the amount added
   */
  virtual void notifyIncrement(uint64_t delta) PURE;

  /**
   * Called when a gauge is decremented.
   * @param delta the amount subtracted
   */
  virtual void notifyDecrement(uint64_t delta) PURE;

  /**
   * Called when a gauge value is set directly.
   * @param old_value the previous value
   * @param new_value the new value
   */
  virtual void notifySet(uint64_t old_value, uint64_t new_value) PURE;

  /**
   * Called when a counter is reset (latch operation).
   * For gauges, this is called when sub() brings the value to 0.
   * @param old_value the value before reset
   */
  virtual void notifyReset(uint64_t old_value) PURE;

  /**
   * Called when a metric is added to an aggregation (initial value capture).
   * @param initial_value the current value of the metric when added
   */
  virtual void notifyAdded(uint64_t initial_value) PURE;

  /**
   * Called when a metric is removed from an aggregation.
   * @param final_value the value of the metric when removed
   */
  virtual void notifyRemoved(uint64_t final_value) PURE;
};

} // namespace Stats
} // namespace Envoy
