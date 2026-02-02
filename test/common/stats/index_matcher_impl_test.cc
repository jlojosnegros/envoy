#include "source/common/stats/index_matcher_impl.h"

#include "source/common/stats/symbol_table.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Stats {
namespace {

class IndexMatcherTest : public testing::Test {
protected:
  IndexMatcherTest() : pool_(symbol_table_) {}

  StatName makeStatName(const std::string& name) { return pool_.add(name); }

  SymbolTableImpl symbol_table_;
  StatNamePool pool_;
};

// PrefixSuffixIndexMatcher tests

TEST_F(IndexMatcherTest, PrefixOnlyMatches) {
  PrefixSuffixIndexMatcher matcher("cluster.", "");

  EXPECT_TRUE(matcher.matches("cluster.foo.upstream_rq"));
  EXPECT_TRUE(matcher.matches("cluster.bar"));
  EXPECT_TRUE(matcher.matches("cluster."));
  EXPECT_FALSE(matcher.matches("http.downstream_rq"));
  EXPECT_FALSE(matcher.matches("cluste.foo"));
}

TEST_F(IndexMatcherTest, SuffixOnlyMatches) {
  PrefixSuffixIndexMatcher matcher("", ".upstream_rq");

  EXPECT_TRUE(matcher.matches("cluster.foo.upstream_rq"));
  EXPECT_TRUE(matcher.matches("bar.upstream_rq"));
  EXPECT_TRUE(matcher.matches(".upstream_rq"));
  EXPECT_FALSE(matcher.matches("cluster.foo.downstream_rq"));
  EXPECT_FALSE(matcher.matches("upstream_rq_total"));
}

TEST_F(IndexMatcherTest, PrefixAndSuffixMatches) {
  PrefixSuffixIndexMatcher matcher("cluster.", ".upstream_rq");

  EXPECT_TRUE(matcher.matches("cluster.foo.upstream_rq"));
  EXPECT_TRUE(matcher.matches("cluster.bar.baz.upstream_rq"));
  EXPECT_TRUE(matcher.matches("cluster..upstream_rq"));
  EXPECT_FALSE(matcher.matches("cluster.foo.downstream_rq"));
  EXPECT_FALSE(matcher.matches("http.foo.upstream_rq"));
  EXPECT_FALSE(matcher.matches("cluster.foo"));
}

TEST_F(IndexMatcherTest, EmptyPrefixAndSuffixMatchesAll) {
  PrefixSuffixIndexMatcher matcher("", "");

  EXPECT_TRUE(matcher.matches("anything"));
  EXPECT_TRUE(matcher.matches("cluster.foo.bar"));
  EXPECT_TRUE(matcher.matches(""));
}

TEST_F(IndexMatcherTest, PrefixSuffixDescribe) {
  EXPECT_EQ("prefix='cluster.'", PrefixSuffixIndexMatcher("cluster.", "").describe());
  EXPECT_EQ("suffix='.upstream_rq'", PrefixSuffixIndexMatcher("", ".upstream_rq").describe());
  EXPECT_EQ("prefix='cluster.' AND suffix='.upstream_rq'",
            PrefixSuffixIndexMatcher("cluster.", ".upstream_rq").describe());
  EXPECT_EQ("all", PrefixSuffixIndexMatcher("", "").describe());
}

TEST_F(IndexMatcherTest, PrefixSuffixMatchesStatName) {
  PrefixSuffixIndexMatcher matcher("cluster.", ".upstream_rq");

  EXPECT_TRUE(matcher.matchesStatName(makeStatName("cluster.foo.upstream_rq"), symbol_table_));
  EXPECT_FALSE(matcher.matchesStatName(makeStatName("http.foo.upstream_rq"), symbol_table_));
}

// RegexIndexMatcher tests

TEST_F(IndexMatcherTest, RegexMatches) {
  RegexIndexMatcher matcher("cluster\\.[^.]+\\.upstream_rq");

  EXPECT_TRUE(matcher.matches("cluster.foo.upstream_rq"));
  EXPECT_TRUE(matcher.matches("cluster.bar.upstream_rq"));
  EXPECT_FALSE(matcher.matches("cluster.foo.bar.upstream_rq"));
  EXPECT_FALSE(matcher.matches("http.foo.upstream_rq"));
}

TEST_F(IndexMatcherTest, RegexMatchesComplex) {
  RegexIndexMatcher matcher("^(cluster|listener)\\..+\\.(upstream|downstream)_rq$");

  EXPECT_TRUE(matcher.matches("cluster.foo.upstream_rq"));
  EXPECT_TRUE(matcher.matches("listener.bar.downstream_rq"));
  EXPECT_TRUE(matcher.matches("cluster.a.b.c.upstream_rq"));
  EXPECT_FALSE(matcher.matches("http.foo.upstream_rq"));
  EXPECT_FALSE(matcher.matches("cluster.foo.total_rq"));
}

TEST_F(IndexMatcherTest, RegexDescribe) {
  RegexIndexMatcher matcher("foo.*bar");
  EXPECT_EQ("regex='foo.*bar'", matcher.describe());
}

// OrIndexMatcher tests

TEST_F(IndexMatcherTest, OrMatcherMatchesAny) {
  std::vector<IndexMatcherPtr> matchers;
  matchers.push_back(std::make_unique<PrefixSuffixIndexMatcher>("cluster.", ""));
  matchers.push_back(std::make_unique<PrefixSuffixIndexMatcher>("listener.", ""));
  OrIndexMatcher or_matcher(std::move(matchers));

  EXPECT_TRUE(or_matcher.matches("cluster.foo"));
  EXPECT_TRUE(or_matcher.matches("listener.bar"));
  EXPECT_FALSE(or_matcher.matches("http.baz"));
}

TEST_F(IndexMatcherTest, OrMatcherWithMixedTypes) {
  std::vector<IndexMatcherPtr> matchers;
  matchers.push_back(std::make_unique<PrefixSuffixIndexMatcher>("cluster.", ""));
  matchers.push_back(std::make_unique<RegexIndexMatcher>("^http\\..+\\.rq_total$"));
  OrIndexMatcher or_matcher(std::move(matchers));

  EXPECT_TRUE(or_matcher.matches("cluster.foo.bar"));
  EXPECT_TRUE(or_matcher.matches("http.downstream.rq_total"));
  EXPECT_FALSE(or_matcher.matches("listener.foo"));
  EXPECT_FALSE(or_matcher.matches("http.downstream.rq_error"));
}

TEST_F(IndexMatcherTest, OrMatcherEmpty) {
  std::vector<IndexMatcherPtr> matchers;
  OrIndexMatcher or_matcher(std::move(matchers));

  // Empty OR matcher should match nothing
  EXPECT_FALSE(or_matcher.matches("anything"));
  EXPECT_FALSE(or_matcher.matches(""));
}

TEST_F(IndexMatcherTest, OrMatcherDescribe) {
  std::vector<IndexMatcherPtr> matchers;
  matchers.push_back(std::make_unique<PrefixSuffixIndexMatcher>("cluster.", ""));
  matchers.push_back(std::make_unique<PrefixSuffixIndexMatcher>("", ".rq"));
  OrIndexMatcher or_matcher(std::move(matchers));

  EXPECT_EQ("(prefix='cluster.' OR suffix='.rq')", or_matcher.describe());
}

TEST_F(IndexMatcherTest, OrMatcherMatchesStatName) {
  std::vector<IndexMatcherPtr> matchers;
  matchers.push_back(std::make_unique<PrefixSuffixIndexMatcher>("cluster.", ""));
  matchers.push_back(std::make_unique<PrefixSuffixIndexMatcher>("listener.", ""));
  OrIndexMatcher or_matcher(std::move(matchers));

  EXPECT_TRUE(or_matcher.matchesStatName(makeStatName("cluster.foo"), symbol_table_));
  EXPECT_TRUE(or_matcher.matchesStatName(makeStatName("listener.bar"), symbol_table_));
  EXPECT_FALSE(or_matcher.matchesStatName(makeStatName("http.baz"), symbol_table_));
}

TEST_F(IndexMatcherTest, OrMatcherCount) {
  std::vector<IndexMatcherPtr> matchers;
  matchers.push_back(std::make_unique<PrefixSuffixIndexMatcher>("a.", ""));
  matchers.push_back(std::make_unique<PrefixSuffixIndexMatcher>("b.", ""));
  matchers.push_back(std::make_unique<PrefixSuffixIndexMatcher>("c.", ""));
  OrIndexMatcher or_matcher(std::move(matchers));

  EXPECT_EQ(3, or_matcher.matcherCount());
}

// Edge cases

TEST_F(IndexMatcherTest, PrefixLongerThanName) {
  PrefixSuffixIndexMatcher matcher("verylongprefix", "");

  EXPECT_FALSE(matcher.matches("short"));
  EXPECT_FALSE(matcher.matches("verylong"));
  EXPECT_TRUE(matcher.matches("verylongprefix"));
  EXPECT_TRUE(matcher.matches("verylongprefixandmore"));
}

TEST_F(IndexMatcherTest, SuffixLongerThanName) {
  PrefixSuffixIndexMatcher matcher("", "verylongsuffix");

  EXPECT_FALSE(matcher.matches("short"));
  EXPECT_FALSE(matcher.matches("suffix"));
  EXPECT_TRUE(matcher.matches("verylongsuffix"));
  EXPECT_TRUE(matcher.matches("prefixverylongsuffix"));
}

TEST_F(IndexMatcherTest, OverlappingPrefixSuffix) {
  // Prefix and suffix overlap in the middle
  PrefixSuffixIndexMatcher matcher("abc", "bcd");

  EXPECT_TRUE(matcher.matches("abcd"));
  EXPECT_TRUE(matcher.matches("abcXbcd"));
  EXPECT_FALSE(matcher.matches("abc"));
  EXPECT_FALSE(matcher.matches("bcd"));
}

} // namespace
} // namespace Stats
} // namespace Envoy
