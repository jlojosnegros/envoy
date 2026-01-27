#pragma once

#include <cstdint>

#include "envoy/api/api.h"

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
 * Looks for gauges matching:
 *   - *.downstream_rq_active (downstream: HTTP, Redis Proxy, Generic Proxy, etc.)
 *   - cluster.*.upstream_rq_active (upstream)
 */
class ActivityStatsReaderImpl : public ActivityStatsReader {
public:
  explicit ActivityStatsReaderImpl(Api::Api& api);

  uint64_t downstreamActiveRequests() const override;
  uint64_t upstreamActiveRequests() const override;

private:
  Api::Api& api_;
};

} // namespace IdleActivityMonitor
} // namespace ResourceMonitors
} // namespace Extensions
} // namespace Envoy
