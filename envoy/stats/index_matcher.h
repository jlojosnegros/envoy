#pragma once

#include <memory>
#include <string>

#include "envoy/common/pure.h"

namespace Envoy {
namespace Stats {

class StatName;
class SymbolTable;

/**
 * Interface for determining if a metric should be included in an index.
 *
 * Unlike StatsMatcher (which rejects stats from being created), IndexMatcher
 * determines which stats should be included in a secondary index for efficient
 * lookup and aggregation.
 *
 * IndexMatcher implementations should be thread-safe for concurrent reads.
 */
class IndexMatcher {
public:
  virtual ~IndexMatcher() = default;

  /**
   * Determines if a metric name matches this index criteria using a string name.
   *
   * @param name the full stat name as a string.
   * @return true if the metric should be included in the index.
   */
  virtual bool matches(const std::string& name) const PURE;

  /**
   * Determines if a metric name matches this index criteria using a StatName.
   *
   * This method provides an optimization path for cases where the StatName is
   * already available and can be matched without string conversion. The default
   * implementation converts StatName to string and calls matches(const std::string&).
   *
   * @param name the stat name in encoded form.
   * @param symbol_table the symbol table used to decode the name if needed.
   * @return true if the metric should be included in the index.
   */
  virtual bool matchesStatName(StatName name, const SymbolTable& symbol_table) const;

  /**
   * Returns a human-readable description of the match criteria.
   * Useful for debugging and admin interfaces.
   *
   * @return a string describing what this matcher matches.
   */
  virtual std::string describe() const PURE;
};

using IndexMatcherPtr = std::unique_ptr<IndexMatcher>;
using IndexMatcherConstPtr = std::unique_ptr<const IndexMatcher>;

} // namespace Stats
} // namespace Envoy
