#include "source/common/stats/index_matcher_impl.h"

#include "source/common/common/fmt.h"

#include "absl/strings/str_cat.h"

namespace Envoy {
namespace Stats {

// Default implementation for IndexMatcher::matchesStatName
bool IndexMatcher::matchesStatName(StatName name, const SymbolTable& symbol_table) const {
  return matches(symbol_table.toString(name));
}

// PrefixSuffixIndexMatcher implementation
PrefixSuffixIndexMatcher::PrefixSuffixIndexMatcher(std::string prefix, std::string suffix)
    : prefix_(std::move(prefix)), suffix_(std::move(suffix)) {}

bool PrefixSuffixIndexMatcher::matches(const std::string& name) const {
  if (!prefix_.empty() && !absl::StartsWith(name, prefix_)) {
    return false;
  }
  if (!suffix_.empty() && !absl::EndsWith(name, suffix_)) {
    return false;
  }
  return true;
}

bool PrefixSuffixIndexMatcher::matchesStatName(StatName name,
                                               const SymbolTable& symbol_table) const {
  // For prefix/suffix matching, we need the string representation
  // TODO(indexedstats): Consider optimizing for prefix matching using StatName tokens
  return matches(symbol_table.toString(name));
}

std::string PrefixSuffixIndexMatcher::describe() const {
  if (!prefix_.empty() && !suffix_.empty()) {
    return absl::StrCat("prefix='", prefix_, "' AND suffix='", suffix_, "'");
  } else if (!prefix_.empty()) {
    return absl::StrCat("prefix='", prefix_, "'");
  } else if (!suffix_.empty()) {
    return absl::StrCat("suffix='", suffix_, "'");
  }
  return "all";
}

// RegexIndexMatcher implementation
RegexIndexMatcher::RegexIndexMatcher(const std::string& pattern) : pattern_(pattern) {
  regex_ = Regex::Utility::parseStdRegex(pattern);
}

bool RegexIndexMatcher::matches(const std::string& name) const { return regex_->match(name); }

std::string RegexIndexMatcher::describe() const { return absl::StrCat("regex='", pattern_, "'"); }

// OrIndexMatcher implementation
OrIndexMatcher::OrIndexMatcher(std::vector<IndexMatcherPtr> matchers)
    : matchers_(std::move(matchers)) {}

bool OrIndexMatcher::matches(const std::string& name) const {
  for (const auto& matcher : matchers_) {
    if (matcher->matches(name)) {
      return true;
    }
  }
  return false;
}

bool OrIndexMatcher::matchesStatName(StatName name, const SymbolTable& symbol_table) const {
  for (const auto& matcher : matchers_) {
    if (matcher->matchesStatName(name, symbol_table)) {
      return true;
    }
  }
  return false;
}

std::string OrIndexMatcher::describe() const {
  std::vector<std::string> descriptions;
  descriptions.reserve(matchers_.size());
  for (const auto& matcher : matchers_) {
    descriptions.push_back(matcher->describe());
  }
  return absl::StrCat("(", absl::StrJoin(descriptions, " OR "), ")");
}

} // namespace Stats
} // namespace Envoy
