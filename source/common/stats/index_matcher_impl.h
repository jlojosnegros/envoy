#pragma once

#include <memory>
#include <string>
#include <vector>

#include "envoy/stats/index_matcher.h"

#include "source/common/common/regex.h"
#include "source/common/stats/symbol_table.h"

#include "absl/strings/string_view.h"

namespace Envoy {
namespace Stats {

/**
 * IndexMatcher that matches based on prefix and/or suffix of the stat name.
 * Provides O(1) matching performance for simple string prefix/suffix checks.
 */
class PrefixSuffixIndexMatcher : public IndexMatcher {
public:
  /**
   * Creates a matcher that checks for prefix and/or suffix.
   * @param prefix the prefix to match (empty string matches any prefix).
   * @param suffix the suffix to match (empty string matches any suffix).
   */
  PrefixSuffixIndexMatcher(std::string prefix, std::string suffix);

  // IndexMatcher
  bool matches(const std::string& name) const override;
  bool matchesStatName(StatName name, const SymbolTable& symbol_table) const override;
  std::string describe() const override;

  const std::string& prefix() const { return prefix_; }
  const std::string& suffix() const { return suffix_; }

private:
  const std::string prefix_;
  const std::string suffix_;
};

/**
 * IndexMatcher that uses regular expressions for flexible matching.
 * Uses RE2 for efficient regex matching.
 */
class RegexIndexMatcher : public IndexMatcher {
public:
  /**
   * Creates a matcher that uses a regex pattern.
   * @param pattern the RE2 regex pattern to match against.
   * @throws EnvoyException if the regex pattern is invalid.
   */
  explicit RegexIndexMatcher(const std::string& pattern);

  // IndexMatcher
  bool matches(const std::string& name) const override;
  std::string describe() const override;

private:
  Regex::CompiledMatcherPtr regex_;
  const std::string pattern_;
};

/**
 * IndexMatcher that combines multiple matchers with OR semantics.
 * A stat matches if any of the child matchers match.
 */
class OrIndexMatcher : public IndexMatcher {
public:
  /**
   * Creates a composite matcher from a list of matchers.
   * @param matchers the child matchers (takes ownership).
   */
  explicit OrIndexMatcher(std::vector<IndexMatcherPtr> matchers);

  // IndexMatcher
  bool matches(const std::string& name) const override;
  bool matchesStatName(StatName name, const SymbolTable& symbol_table) const override;
  std::string describe() const override;

  size_t matcherCount() const { return matchers_.size(); }

private:
  std::vector<IndexMatcherPtr> matchers_;
};

} // namespace Stats
} // namespace Envoy
