#include "source/extensions/resource_monitors/idle_activity/activity_stats_reader.h"

#include "envoy/stats/stats.h"
#include "envoy/stats/store.h"

#include "absl/strings/match.h"

namespace Envoy {
namespace Extensions {
namespace ResourceMonitors {
namespace IdleActivityMonitor {

ActivityStatsReaderImpl::ActivityStatsReaderImpl(Api::Api& api) : api_(api) {}

uint64_t ActivityStatsReaderImpl::downstreamActiveRequests() const {
  uint64_t total = 0;

  // Iterate over all gauges and sum those matching the pattern for downstream active requests.
  // The pattern is: http.<listener_name>.downstream_rq_active
  api_.rootScope().store().iterate(
      Stats::IterateFn<Stats::Gauge>([&total](const Stats::GaugeSharedPtr& gauge) -> bool {
        const std::string name = gauge->name();
        // Match pattern: starts with "http." and ends with ".downstream_rq_active"
        if (absl::StartsWith(name, "http.") && absl::EndsWith(name, ".downstream_rq_active")) {
          total += gauge->value();
        }
        return true; // Continue iteration
      }));

  return total;
}

uint64_t ActivityStatsReaderImpl::upstreamActiveRequests() const {
  uint64_t total = 0;

  // Iterate over all gauges and sum those matching the pattern for upstream active requests.
  // The pattern is: cluster.<cluster_name>.upstream_rq_active
  api_.rootScope().store().iterate(
      Stats::IterateFn<Stats::Gauge>([&total](const Stats::GaugeSharedPtr& gauge) -> bool {
        const std::string name = gauge->name();
        // Match pattern: starts with "cluster." and ends with ".upstream_rq_active"
        if (absl::StartsWith(name, "cluster.") && absl::EndsWith(name, ".upstream_rq_active")) {
          total += gauge->value();
        }
        return true; // Continue iteration
      }));

  return total;
}

} // namespace IdleActivityMonitor
} // namespace ResourceMonitors
} // namespace Extensions
} // namespace Envoy
