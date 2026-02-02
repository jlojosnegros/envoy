#pragma once

#include "envoy/config/metrics/v3/stats_index.pb.h"

#include "source/common/stats/indexed_stats_store.h"

namespace Envoy {
namespace Stats {

/**
 * Factory for creating stats indices from protobuf configuration.
 *
 * This factory handles the conversion from proto configuration to
 * IndexMatcher and StatsIndex objects, enabling configuration-driven
 * index creation.
 */
class StatsIndexFactory {
public:
  /**
   * Creates an IndexMatcher from a StatsIndexConfig.
   * @param config the proto configuration
   * @return the created matcher
   * @throws EnvoyException if the configuration is invalid
   */
  static IndexMatcherPtr createMatcher(const envoy::config::metrics::v3::StatsIndexConfig& config);

  /**
   * Creates an IndexMatcher from a PrefixSuffixMatcher proto.
   * @param config the prefix/suffix matcher configuration
   * @return the created matcher
   */
  static IndexMatcherPtr
  createPrefixSuffixMatcher(const envoy::config::metrics::v3::PrefixSuffixMatcher& config);

  /**
   * Creates an IndexMatcher from a StringMatcher proto.
   * @param config the string matcher configuration
   * @return the created matcher
   * @throws EnvoyException if the configuration is invalid
   */
  static IndexMatcherPtr createStringMatcher(const envoy::type::matcher::v3::StringMatcher& config);

  /**
   * Creates all indices from a StatsIndicesConfig and registers them with the store.
   *
   * This method is intended to be called at bootstrap time before any metrics
   * are created, allowing the most efficient indexing path.
   *
   * @param store the IndexedStatsStore to register indices with
   * @param config the proto configuration containing all index definitions
   */
  static void createIndicesFromConfig(IndexedStatsStore& store,
                                       const envoy::config::metrics::v3::StatsIndicesConfig& config);

  /**
   * Creates all indices from a StatsIndicesConfig and registers them, scanning
   * existing metrics.
   *
   * This method is intended for runtime registration of indices when metrics
   * may already exist.
   *
   * @param store the IndexedStatsStore to register indices with
   * @param config the proto configuration containing all index definitions
   */
  static void createIndicesFromConfigWithExisting(
      IndexedStatsStore& store, const envoy::config::metrics::v3::StatsIndicesConfig& config);
};

} // namespace Stats
} // namespace Envoy
