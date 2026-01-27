#include "source/extensions/resource_monitors/idle_activity/idle_activity_monitor.h"

#include "envoy/extensions/resource_monitors/idle_activity/v3/idle_activity.pb.h"

#include "source/common/common/assert.h"
#include "source/common/protobuf/utility.h"

namespace Envoy {
namespace Extensions {
namespace ResourceMonitors {
namespace IdleActivityMonitor {

IdleActivityMonitor::IdleActivityMonitor(
    const envoy::extensions::resource_monitors::idle_activity::v3::IdleActivityConfig& config,
    TimeSource& time_source, std::unique_ptr<ActivityStatsReader> stats_reader)
    : active_requests_threshold_(config.active_requests_threshold()),
      downstream_threshold_(config.downstream_requests_threshold() > 0
                                ? absl::make_optional(config.downstream_requests_threshold())
                                : absl::nullopt),
      upstream_threshold_(config.upstream_requests_threshold() > 0
                              ? absl::make_optional(config.upstream_requests_threshold())
                              : absl::nullopt),
      sustained_duration_(std::chrono::milliseconds(
          DurationUtil::durationToMilliseconds(config.sustained_idle_duration()))),
      time_source_(time_source), stats_reader_(std::move(stats_reader)) {
  ASSERT(sustained_duration_.count() >= 1000,
         "sustained_idle_duration must be at least 1 second");
}

void IdleActivityMonitor::updateResourceUsage(Server::ResourceUpdateCallbacks& callbacks) {
  const MonotonicTime now = time_source_.monotonicTime();
  const bool below_threshold = isBelowThreshold();

  Server::ResourceUsage usage;

  if (below_threshold) {
    if (!idle_start_time_.has_value()) {
      // Transition to idle state - start tracking duration
      idle_start_time_ = now;
      usage.resource_pressure_ = 0.0;
      ENVOY_LOG_MISC(debug, "IdleActivityMonitor: entering idle state");
    } else {
      // Already in idle state - check if sustained duration has elapsed
      const auto idle_duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(now - *idle_start_time_);
      if (idle_duration >= sustained_duration_) {
        // Sustained idle - report high pressure to trigger action
        usage.resource_pressure_ = 1.0;
        ENVOY_LOG_MISC(debug,
                       "IdleActivityMonitor: sustained idle for {}ms, reporting pressure=1.0",
                       idle_duration.count());
      } else {
        // Still waiting for sustained duration
        usage.resource_pressure_ = 0.0;
        ENVOY_LOG_MISC(trace, "IdleActivityMonitor: idle for {}ms, waiting for {}ms",
                       idle_duration.count(), sustained_duration_.count());
      }
    }
  } else {
    // Above threshold - reset idle tracking
    if (idle_start_time_.has_value()) {
      ENVOY_LOG_MISC(debug, "IdleActivityMonitor: exiting idle state due to activity");
    }
    idle_start_time_.reset();
    usage.resource_pressure_ = 0.0;
  }

  callbacks.onSuccess(usage);
}

bool IdleActivityMonitor::isBelowThreshold() const {
  if (stats_reader_ == nullptr) {
    // No stats reader available (should not happen in production)
    return false;
  }

  const uint64_t downstream = stats_reader_->downstreamActiveRequests();
  const uint64_t upstream = stats_reader_->upstreamActiveRequests();
  const uint64_t total = downstream + upstream;

  // Check total threshold
  if (total < active_requests_threshold_) {
    ENVOY_LOG_MISC(trace,
                   "IdleActivityMonitor: total={} < threshold={}, considering idle (total check)",
                   total, active_requests_threshold_);
    return true;
  }

  // Check separate downstream threshold if configured
  if (downstream_threshold_.has_value() && downstream < *downstream_threshold_) {
    ENVOY_LOG_MISC(
        trace,
        "IdleActivityMonitor: downstream={} < threshold={}, considering idle (downstream check)",
        downstream, *downstream_threshold_);
    return true;
  }

  // Check separate upstream threshold if configured
  if (upstream_threshold_.has_value() && upstream < *upstream_threshold_) {
    ENVOY_LOG_MISC(
        trace,
        "IdleActivityMonitor: upstream={} < threshold={}, considering idle (upstream check)",
        upstream, *upstream_threshold_);
    return true;
  }

  ENVOY_LOG_MISC(trace,
                 "IdleActivityMonitor: total={}, downstream={}, upstream={}, not idle (above all "
                 "thresholds)",
                 total, downstream, upstream);
  return false;
}

} // namespace IdleActivityMonitor
} // namespace ResourceMonitors
} // namespace Extensions
} // namespace Envoy
