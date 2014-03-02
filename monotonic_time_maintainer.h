// TODO(dkorolev): Use the generic 'ms' field extractor.
// TODO(dkorolev): Also blend felicity::date_now() into this logic. They belong together.

// Implements a helper class that ensures the input stream of log messages is increasing in time.
// The entries that go back in time by less than --acceptable_time_skew_ms[=50ms by default] are forced to last seen timestamp.
// The entires that go further back in time are discarded.
// Rolling counters of seen, passed, adjusted and dropped entries are kept and can be exported in a Thrift-friendly format.

#ifndef MONOTONIC_TIME_MAINTAINER_H
#define MONOTONIC_TIME_MAINTAINER_H

#include <mutex>

#include "sliding_window_stats.h"

#include "../felicity/all.h"

DEFINE_int64(acceptable_time_skew_ms, 50, "Time going back for more than this is not acceptable. The rest is pushed forward.");

namespace marvin {

template<typename T_TIMESTAMP = uint64_t> class monotonic_time_maintainer {
 public:
  monotonic_time_maintainer() : max_skew_(FLAGS_acceptable_time_skew_ms) {
  }

  monotonic_time_maintainer(T_TIMESTAMP max_skew) : max_skew_(max_skew) {
  }

  monotonic_time_maintainer(const monotonic_time_maintainer&) = delete;
  void operator=(const monotonic_time_maintainer&) = delete;

  // Returns true if the entry should be used.
  // Potentially modifies the entry's 'ms' field to ensure time is non-decreasing.
  // Only does so in case the skew is less than --acceptable_time_skew_ms, defaults to 50ms.
  template<typename T> bool use(T& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    const T_TIMESTAMP wall_time = felicity::date_now();
    const T_TIMESTAMP ms = entry.ms;
    assert(ms > 0);
    if (virgin_) {
      virgin_ = false;
      last_timestamp_ = ms;
    }
    stats_.seen.add(wall_time);
    if (ms >= last_timestamp_) {
      last_timestamp_ = ms;
      stats_.passed.add(wall_time);
      return true;
    } else if (ms >= last_timestamp_ - max_skew_) {
      stats_.adjusted.add(wall_time);
      entry.ms = last_timestamp_;
      return true;
    } else {
      stats_.dropped.add(wall_time);
      return false;
    }
  }

  bool relax(T_TIMESTAMP timestamp) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (timestamp < last_timestamp_) {
      return false;
    } else {
      last_timestamp_ = timestamp;
      return true;
    }
  }

  T_TIMESTAMP last_timestamp() const {
    return last_timestamp_;
  }

  struct rolling_stats {
    sliding_windows<dummy_stats<T_TIMESTAMP>> seen;
    sliding_windows<dummy_stats<T_TIMESTAMP>> passed;
    sliding_windows<dummy_stats<T_TIMESTAMP>> adjusted;
    sliding_windows<dummy_stats<T_TIMESTAMP>> dropped;
    template<typename T> void export_monotonicity_log(T& output, T_TIMESTAMP passed_in_now = 0) {
      const T_TIMESTAMP now = passed_in_now ? passed_in_now : felicity::date_now();
      seen.export_counters(output.seen, now);
      passed.export_counters(output.passed, now);
      adjusted.export_counters(output.adjusted, now);
      dropped.export_counters(output.dropped, now);
    }
  };

  template<typename T> void export_monotonicity_log(T& output, T_TIMESTAMP passed_in_now = 0) {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.export_monotonicity_log(output, passed_in_now);
  }

 private:
  std::mutex mutex_;
  rolling_stats stats_;
  const T_TIMESTAMP max_skew_;
  bool virgin_ = true;
  T_TIMESTAMP last_timestamp_ = 0;
};

}  // namespace marvin
  
#endif
