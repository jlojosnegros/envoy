#pragma once

#include <chrono>
#include <memory>

#include "envoy/common/time.h"
#include "envoy/extensions/resource_monitors/idle_activity/v3/idle_activity.pb.h"
#include "envoy/server/resource_monitor.h"

#include "source/extensions/resource_monitors/idle_activity/activity_stats_reader.h"

#include "absl/types/optional.h"

namespace Envoy {
namespace Extensions {
namespace ResourceMonitors {
namespace IdleActivityMonitor {

/**
 * Resource monitor that reports high pressure when the system is idle
 * (low active requests) for a sustained duration.
 *
 * This uses "inverted" semantics compared to typical resource monitors:
 * - Low activity (below threshold) for sustained duration -> pressure = 1.0 (trigger action)
 * - High activity (above threshold) or brief idle -> pressure = 0.0 (no trigger)
 *
 * This allows the monitor to trigger actions like shrink_heap when the system
 * is idle, while remaining compatible with the existing overload framework.
 */
class IdleActivityMonitor : public Server::ResourceMonitor {
public:
  IdleActivityMonitor(
      const envoy::extensions::resource_monitors::idle_activity::v3::IdleActivityConfig& config,
      TimeSource& time_source,
      std::unique_ptr<ActivityStatsReader> stats_reader = nullptr);

  void updateResourceUsage(Server::ResourceUpdateCallbacks& callbacks) override;

private:
  /**
   * Determines if the current activity level is below the configured thresholds.
   * @return true if the system should be considered idle.
   */
  bool isBelowThreshold() const;

  // Configuration
  const uint64_t active_requests_threshold_;
  const absl::optional<uint64_t> downstream_threshold_;
  const absl::optional<uint64_t> upstream_threshold_;
  const std::chrono::milliseconds sustained_duration_;

  // Dependencies
  TimeSource& time_source_;
  std::unique_ptr<ActivityStatsReader> stats_reader_;

  // State
  absl::optional<MonotonicTime> idle_start_time_;
};

} // namespace IdleActivityMonitor
} // namespace ResourceMonitors
} // namespace Extensions
} // namespace Envoy
