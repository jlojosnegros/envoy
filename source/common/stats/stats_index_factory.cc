#include "source/common/stats/stats_index_factory.h"

#include "source/common/common/assert.h"
#include "source/common/common/regex.h"

#include "absl/strings/str_cat.h"

namespace Envoy {
namespace Stats {

IndexMatcherPtr
StatsIndexFactory::createMatcher(const envoy::config::metrics::v3::StatsIndexConfig& config) {
  switch (config.matcher_case()) {
  case envoy::config::metrics::v3::StatsIndexConfig::MatcherCase::kPrefixSuffix:
    return createPrefixSuffixMatcher(config.prefix_suffix());
  case envoy::config::metrics::v3::StatsIndexConfig::MatcherCase::kStringMatcher:
    return createStringMatcher(config.string_matcher());
  case envoy::config::metrics::v3::StatsIndexConfig::MatcherCase::MATCHER_NOT_SET:
    PANIC("StatsIndexConfig matcher not set");
  }
  PANIC_DUE_TO_CORRUPT_ENUM;
}

IndexMatcherPtr StatsIndexFactory::createPrefixSuffixMatcher(
    const envoy::config::metrics::v3::PrefixSuffixMatcher& config) {
  return std::make_unique<PrefixSuffixIndexMatcher>(config.prefix(), config.suffix());
}

IndexMatcherPtr
StatsIndexFactory::createStringMatcher(const envoy::type::matcher::v3::StringMatcher& config) {
  // Convert StringMatcher to appropriate IndexMatcher
  switch (config.match_pattern_case()) {
  case envoy::type::matcher::v3::StringMatcher::MatchPatternCase::kExact:
    // Exact match: use prefix=suffix=exact for exact matching behavior
    // Actually, prefix+suffix doesn't work for exact match. Use regex.
    return std::make_unique<RegexIndexMatcher>(
        absl::StrCat("^", Regex::Utility::replaceAllToLower(config.exact()), "$"));
  case envoy::type::matcher::v3::StringMatcher::MatchPatternCase::kPrefix:
    return std::make_unique<PrefixSuffixIndexMatcher>(config.prefix(), "");
  case envoy::type::matcher::v3::StringMatcher::MatchPatternCase::kSuffix:
    return std::make_unique<PrefixSuffixIndexMatcher>("", config.suffix());
  case envoy::type::matcher::v3::StringMatcher::MatchPatternCase::kSafeRegex:
    return std::make_unique<RegexIndexMatcher>(config.safe_regex().regex());
  case envoy::type::matcher::v3::StringMatcher::MatchPatternCase::kContains:
    // Contains: use regex with .* on both sides
    return std::make_unique<RegexIndexMatcher>(
        absl::StrCat(".*", Regex::Utility::replaceAllToLower(config.contains()), ".*"));
  case envoy::type::matcher::v3::StringMatcher::MatchPatternCase::MATCH_PATTERN_NOT_SET:
    PANIC("StringMatcher match_pattern not set");
  }
  PANIC_DUE_TO_CORRUPT_ENUM;
}

void StatsIndexFactory::createIndicesFromConfig(
    IndexedStatsStore& store, const envoy::config::metrics::v3::StatsIndicesConfig& config) {
  for (const auto& index_config : config.indices()) {
    auto matcher = createMatcher(index_config);

    switch (index_config.metric_type()) {
    case envoy::config::metrics::v3::StatsIndexConfig::GAUGE:
      store.registerGaugeIndex(index_config.name(), std::move(matcher));
      break;
    case envoy::config::metrics::v3::StatsIndexConfig::COUNTER:
      store.registerCounterIndex(index_config.name(), std::move(matcher));
      break;
    case envoy::config::metrics::v3::StatsIndexConfig::METRIC_TYPE_UNSPECIFIED:
      PANIC(absl::StrCat("StatsIndexConfig '", index_config.name(),
                          "' has unspecified metric_type"));
    default:
      PANIC_DUE_TO_CORRUPT_ENUM;
    }
  }
}

void StatsIndexFactory::createIndicesFromConfigWithExisting(
    IndexedStatsStore& store, const envoy::config::metrics::v3::StatsIndicesConfig& config) {
  for (const auto& index_config : config.indices()) {
    auto matcher = createMatcher(index_config);

    switch (index_config.metric_type()) {
    case envoy::config::metrics::v3::StatsIndexConfig::GAUGE:
      store.registerGaugeIndexWithExisting(index_config.name(), std::move(matcher));
      break;
    case envoy::config::metrics::v3::StatsIndexConfig::COUNTER:
      store.registerCounterIndexWithExisting(index_config.name(), std::move(matcher));
      break;
    case envoy::config::metrics::v3::StatsIndexConfig::METRIC_TYPE_UNSPECIFIED:
      PANIC(absl::StrCat("StatsIndexConfig '", index_config.name(),
                          "' has unspecified metric_type"));
    default:
      PANIC_DUE_TO_CORRUPT_ENUM;
    }
  }
}

} // namespace Stats
} // namespace Envoy
