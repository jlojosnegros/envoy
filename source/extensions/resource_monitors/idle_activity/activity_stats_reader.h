#pragma once

#include <cstdint>

#include "envoy/api/api.h"
#include "envoy/stats/stats.h"

namespace Envoy {
namespace Extensions {
namespace ResourceMonitors {
namespace IdleActivityMonitor {

/**
 * Interface for reading active request statistics.
 * This abstraction allows for mocking in tests.
 */
class ActivityStatsReader {
public:
  virtual ~ActivityStatsReader() = default;

  /**
   * @return the total number of active downstream requests across all listeners.
   */
  virtual uint64_t downstreamActiveRequests() const = 0;

  /**
   * @return the total number of active upstream requests across all clusters.
   */
  virtual uint64_t upstreamActiveRequests() const = 0;

  /**
   * @return the total number of active requests (downstream + upstream).
   */
  uint64_t totalActiveRequests() const {
    return downstreamActiveRequests() + upstreamActiveRequests();
  }
};

/**
 * Implementation of ActivityStatsReader that reads stats from the Envoy stats system.
 * Uses global server.total_upstream_rq_active and server.total_downstream_rq_active gauges
 * for O(1) lookup instead of iterating over all per-cluster/per-listener gauges.
 *
 * Falls back to iterating if global gauges are not found (backwards compatibility).
 */
class ActivityStatsReaderImpl : public ActivityStatsReader {
public:
  explicit ActivityStatsReaderImpl(Api::Api& api);

  uint64_t downstreamActiveRequests() const override;
  uint64_t upstreamActiveRequests() const override;

private:
  /**
   * Initialize gauges by looking them up in the stats store.
   * Called lazily on first access.
   */
  void initializeGauges() const;

  /**
   * Fallback method that iterates over all gauges (O(n)).
   * Used when global gauges are not available.
   */
  uint64_t downstreamActiveRequestsFallback() const;
  uint64_t upstreamActiveRequestsFallback() const;

  Api::Api& api_;
  mutable bool gauges_initialized_{false};
  mutable Stats::Gauge* upstream_gauge_{nullptr};
  mutable Stats::Gauge* downstream_gauge_{nullptr};
};

} // namespace IdleActivityMonitor
} // namespace ResourceMonitors
} // namespace Extensions
} // namespace Envoy
