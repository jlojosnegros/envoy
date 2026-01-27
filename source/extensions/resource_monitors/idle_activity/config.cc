#include "source/extensions/resource_monitors/idle_activity/config.h"

#include "envoy/extensions/resource_monitors/idle_activity/v3/idle_activity.pb.h"
#include "envoy/extensions/resource_monitors/idle_activity/v3/idle_activity.pb.validate.h"
#include "envoy/registry/registry.h"

#include "source/extensions/resource_monitors/idle_activity/activity_stats_reader.h"
#include "source/extensions/resource_monitors/idle_activity/idle_activity_monitor.h"

namespace Envoy {
namespace Extensions {
namespace ResourceMonitors {
namespace IdleActivityMonitor {

Server::ResourceMonitorPtr IdleActivityMonitorFactory::createResourceMonitorFromProtoTyped(
    const envoy::extensions::resource_monitors::idle_activity::v3::IdleActivityConfig& config,
    Server::Configuration::ResourceMonitorFactoryContext& context) {
  return std::make_unique<IdleActivityMonitor>(
      config, context.api().timeSource(),
      std::make_unique<ActivityStatsReaderImpl>(context.api()));
}

/**
 * Static registration for the idle activity resource monitor factory. @see RegistryFactory.
 */
REGISTER_FACTORY(IdleActivityMonitorFactory, Server::Configuration::ResourceMonitorFactory);

} // namespace IdleActivityMonitor
} // namespace ResourceMonitors
} // namespace Extensions
} // namespace Envoy
