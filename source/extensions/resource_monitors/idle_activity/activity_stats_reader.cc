#include "source/extensions/resource_monitors/idle_activity/activity_stats_reader.h"

#include "envoy/stats/stats.h"
#include "envoy/stats/store.h"

#include "absl/strings/match.h"

namespace Envoy {
namespace Extensions {
namespace ResourceMonitors {
namespace IdleActivityMonitor {

ActivityStatsReaderImpl::ActivityStatsReaderImpl(Api::Api& api) : api_(api) {}

void ActivityStatsReaderImpl::initializeGauges() const {
  if (gauges_initialized_) {
    return;
  }

  gauges_initialized_ = true;

  // Look up the global gauges for O(1) access
  api_.rootScope().store().iterate(
      Stats::IterateFn<Stats::Gauge>([this](const Stats::GaugeSharedPtr& gauge) -> bool {
        const std::string name = gauge->name();
        if (name == "server.total_upstream_rq_active") {
          upstream_gauge_ = gauge.get();
        } else if (name == "server.total_downstream_rq_active") {
          downstream_gauge_ = gauge.get();
        }
        // Continue until we find both gauges
        return upstream_gauge_ == nullptr || downstream_gauge_ == nullptr;
      }));
}

uint64_t ActivityStatsReaderImpl::downstreamActiveRequests() const {
  initializeGauges();

  if (downstream_gauge_ != nullptr) {
    return downstream_gauge_->value();
  }

  // Fallback to iterating over all gauges if global gauge not found
  return downstreamActiveRequestsFallback();
}

uint64_t ActivityStatsReaderImpl::upstreamActiveRequests() const {
  initializeGauges();

  if (upstream_gauge_ != nullptr) {
    return upstream_gauge_->value();
  }

  // Fallback to iterating over all gauges if global gauge not found
  return upstreamActiveRequestsFallback();
}

uint64_t ActivityStatsReaderImpl::downstreamActiveRequestsFallback() const {
  uint64_t total = 0;

  // Iterate over all gauges and sum those matching the pattern for downstream active requests.
  // Patterns include:
  //   - http.<listener_name>.downstream_rq_active (HTTP)
  //   - redis.<prefix>.downstream_rq_active (Redis proxy)
  //   - generic_proxy.<prefix>.downstream_rq_active (Generic proxy)
  api_.rootScope().store().iterate(
      Stats::IterateFn<Stats::Gauge>([&total](const Stats::GaugeSharedPtr& gauge) -> bool {
        const std::string name = gauge->name();
        // Match any gauge ending with .downstream_rq_active, excluding the global gauge
        if (absl::EndsWith(name, ".downstream_rq_active") &&
            !absl::StartsWith(name, "server.")) {
          total += gauge->value();
        }
        return true; // Continue iteration
      }));

  return total;
}

uint64_t ActivityStatsReaderImpl::upstreamActiveRequestsFallback() const {
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
