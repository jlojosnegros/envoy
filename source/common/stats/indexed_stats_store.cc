#include "source/common/stats/indexed_stats_store.h"

#include "source/common/common/assert.h"

#include "absl/strings/str_cat.h"

namespace Envoy {
namespace Stats {

IndexedStatsStore::IndexedStatsStore(Store& base_store) : base_store_(base_store) {}

GaugeIndex& IndexedStatsStore::registerGaugeIndex(const std::string& name,
                                                   IndexMatcherPtr matcher) {
  absl::MutexLock lock(&mutex_);
  auto it = gauge_indices_.find(name);
  if (it != gauge_indices_.end()) {
    PANIC(absl::StrCat("Gauge index '", name, "' already exists"));
  }
  auto index = std::make_unique<GaugeIndex>(name, std::move(matcher));
  auto& ref = *index;
  gauge_indices_.emplace(name, std::move(index));
  return ref;
}

GaugeIndex& IndexedStatsStore::registerGaugeIndexWithExisting(const std::string& name,
                                                               IndexMatcherPtr matcher) {
  GaugeIndex& index = registerGaugeIndex(name, std::move(matcher));

  // Populate with existing gauges
  base_store_.forEachGauge([](size_t) {}, [&index](Gauge& gauge) { index.tryAdd(gauge); });

  return index;
}

CounterIndex& IndexedStatsStore::registerCounterIndex(const std::string& name,
                                                       IndexMatcherPtr matcher) {
  absl::MutexLock lock(&mutex_);
  auto it = counter_indices_.find(name);
  if (it != counter_indices_.end()) {
    PANIC(absl::StrCat("Counter index '", name, "' already exists"));
  }
  auto index = std::make_unique<CounterIndex>(name, std::move(matcher));
  auto& ref = *index;
  counter_indices_.emplace(name, std::move(index));
  return ref;
}

CounterIndex& IndexedStatsStore::registerCounterIndexWithExisting(const std::string& name,
                                                                   IndexMatcherPtr matcher) {
  CounterIndex& index = registerCounterIndex(name, std::move(matcher));

  // Populate with existing counters
  base_store_.forEachCounter([](size_t) {},
                              [&index](Counter& counter) { index.tryAdd(counter); });

  return index;
}

GaugeIndex* IndexedStatsStore::getGaugeIndex(const std::string& name) {
  absl::MutexLock lock(&mutex_);
  auto it = gauge_indices_.find(name);
  return it != gauge_indices_.end() ? it->second.get() : nullptr;
}

const GaugeIndex* IndexedStatsStore::getGaugeIndex(const std::string& name) const {
  absl::MutexLock lock(&mutex_);
  auto it = gauge_indices_.find(name);
  return it != gauge_indices_.end() ? it->second.get() : nullptr;
}

CounterIndex* IndexedStatsStore::getCounterIndex(const std::string& name) {
  absl::MutexLock lock(&mutex_);
  auto it = counter_indices_.find(name);
  return it != counter_indices_.end() ? it->second.get() : nullptr;
}

const CounterIndex* IndexedStatsStore::getCounterIndex(const std::string& name) const {
  absl::MutexLock lock(&mutex_);
  auto it = counter_indices_.find(name);
  return it != counter_indices_.end() ? it->second.get() : nullptr;
}

bool IndexedStatsStore::removeGaugeIndex(const std::string& name) {
  absl::MutexLock lock(&mutex_);
  return gauge_indices_.erase(name) > 0;
}

bool IndexedStatsStore::removeCounterIndex(const std::string& name) {
  absl::MutexLock lock(&mutex_);
  return counter_indices_.erase(name) > 0;
}

void IndexedStatsStore::onGaugeCreated(Gauge& gauge) {
  absl::MutexLock lock(&mutex_);
  for (auto& pair : gauge_indices_) {
    pair.second->tryAdd(gauge);
  }
}

void IndexedStatsStore::onCounterCreated(Counter& counter) {
  absl::MutexLock lock(&mutex_);
  for (auto& pair : counter_indices_) {
    pair.second->tryAdd(counter);
  }
}

void IndexedStatsStore::onGaugeDeleted(Gauge& gauge) {
  absl::MutexLock lock(&mutex_);
  for (auto& pair : gauge_indices_) {
    pair.second->remove(gauge);
  }
}

void IndexedStatsStore::onCounterDeleted(Counter& counter) {
  absl::MutexLock lock(&mutex_);
  for (auto& pair : counter_indices_) {
    pair.second->remove(counter);
  }
}

size_t IndexedStatsStore::gaugeIndexCount() const {
  absl::MutexLock lock(&mutex_);
  return gauge_indices_.size();
}

size_t IndexedStatsStore::counterIndexCount() const {
  absl::MutexLock lock(&mutex_);
  return counter_indices_.size();
}

} // namespace Stats
} // namespace Envoy
