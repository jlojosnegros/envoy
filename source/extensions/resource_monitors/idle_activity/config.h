#pragma once

#include "envoy/extensions/resource_monitors/idle_activity/v3/idle_activity.pb.h"
#include "envoy/extensions/resource_monitors/idle_activity/v3/idle_activity.pb.validate.h"
#include "envoy/server/resource_monitor_config.h"

#include "source/extensions/resource_monitors/common/factory_base.h"

namespace Envoy {
namespace Extensions {
namespace ResourceMonitors {
namespace IdleActivityMonitor {

class IdleActivityMonitorFactory
    : public Common::FactoryBase<
          envoy::extensions::resource_monitors::idle_activity::v3::IdleActivityConfig> {
public:
  IdleActivityMonitorFactory() : FactoryBase("envoy.resource_monitors.idle_activity") {}

private:
  Server::ResourceMonitorPtr createResourceMonitorFromProtoTyped(
      const envoy::extensions::resource_monitors::idle_activity::v3::IdleActivityConfig& config,
      Server::Configuration::ResourceMonitorFactoryContext& context) override;
};

} // namespace IdleActivityMonitor
} // namespace ResourceMonitors
} // namespace Extensions
} // namespace Envoy
