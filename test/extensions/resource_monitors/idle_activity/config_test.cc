#include "envoy/extensions/resource_monitors/idle_activity/v3/idle_activity.pb.h"
#include "envoy/extensions/resource_monitors/idle_activity/v3/idle_activity.pb.validate.h"
#include "envoy/registry/registry.h"

#include "source/extensions/resource_monitors/idle_activity/config.h"
#include "source/server/resource_monitor_config_impl.h"

#include "test/mocks/event/mocks.h"
#include "test/mocks/server/options.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace ResourceMonitors {
namespace IdleActivityMonitor {
namespace {

TEST(IdleActivityMonitorFactoryTest, CreateMonitor) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::ResourceMonitorFactory>::getFactory(
          "envoy.resource_monitors.idle_activity");
  EXPECT_NE(factory, nullptr);

  envoy::extensions::resource_monitors::idle_activity::v3::IdleActivityConfig config;
  config.set_active_requests_threshold(5);
  config.mutable_sustained_idle_duration()->set_seconds(30);

  Event::MockDispatcher dispatcher;
  Api::ApiPtr api = Api::createApiForTest();
  Server::MockOptions options;
  Server::Configuration::ResourceMonitorFactoryContextImpl context(
      dispatcher, options, *api, ProtobufMessage::getStrictValidationVisitor());
  auto monitor = factory->createResourceMonitor(config, context);
  EXPECT_NE(monitor, nullptr);
}

TEST(IdleActivityMonitorFactoryTest, CreateMonitorWithSeparateThresholds) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::ResourceMonitorFactory>::getFactory(
          "envoy.resource_monitors.idle_activity");
  EXPECT_NE(factory, nullptr);

  envoy::extensions::resource_monitors::idle_activity::v3::IdleActivityConfig config;
  config.set_active_requests_threshold(10);
  config.set_downstream_requests_threshold(5);
  config.set_upstream_requests_threshold(5);
  config.mutable_sustained_idle_duration()->set_seconds(60);

  Event::MockDispatcher dispatcher;
  Api::ApiPtr api = Api::createApiForTest();
  Server::MockOptions options;
  Server::Configuration::ResourceMonitorFactoryContextImpl context(
      dispatcher, options, *api, ProtobufMessage::getStrictValidationVisitor());
  auto monitor = factory->createResourceMonitor(config, context);
  EXPECT_NE(monitor, nullptr);
}

} // namespace
} // namespace IdleActivityMonitor
} // namespace ResourceMonitors
} // namespace Extensions
} // namespace Envoy
