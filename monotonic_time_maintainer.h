// Implements a helper class that ensures the input stream of log messages is increasing in time.
// The entries that go back in time by less than --acceptable_time_skew_ms[=50ms by default] are forced to last seen timestamp.
// The entires that go further back in time are discarded.
// Rolling counters of seen, passed, adjusted and dropped entries are kept and can be exported in a Thrift-friendly format.

#ifndef MONOTONIC_TIME_MAINTAINER_H
#define MONOTONIC_TIME_MAINTAINER_H

#include "sliding_window_stats.h"

#include "../felicity/all_with_gflags.h"

DEFINE_int64(acceptable_time_skew_ms, 50, "Time going back for more than this is not acceptable. The rest is pushed forward.");

namespace marvin {

class monotonic_time_maintainer {
 public:
  monotonic_time_maintainer() : max_skew_(FLAGS_acceptable_time_skew_ms) {
  }

  monotonic_time_maintainer(uint64_t max_skew) : max_skew_(max_skew) {
  }

  monotonic_time_maintainer(const monotonic_time_maintainer&) = delete;
  void operator=(const monotonic_time_maintainer&) = delete;

  // Returns true if the entry should be used.
  // Potentially modifies the entry's 'ms' field to ensure time is non-decreasing.
  // Only does so in case the skew is less than --acceptable_time_skew_ms, defaults to 50ms.
  template<typename T> bool use(T& entry) {
    const uint64_t wall_time = felicity::date_now();
    const uint64_t ms = entry.ms;
    if (virgin_) {
      virgin_ = false;
      last_ms_ = ms;
    }
    stats_.seen.add(wall_time);
    if (ms >= last_ms_) {
      stats_.passed.add(wall_time);
      return true;
    } else if (ms >= last_ms_ - max_skew_) {
      stats_.adjusted.add(wall_time);
      entry.ms = last_ms_;
      return true;
    } else {
      stats_.dropped.add(wall_time);
      return false;
    }
  }

  struct rolling_stats {
    sliding_windows<dummy_stats<uint64_t>> seen;
    sliding_windows<dummy_stats<uint64_t>> passed;
    sliding_windows<dummy_stats<uint64_t>> adjusted;
    sliding_windows<dummy_stats<uint64_t>> dropped;
    template<typename T> void export_monotonicity_log(T& output, uint64_t passed_in_now = 0) {
      const uint64_t now = passed_in_now ? passed_in_now : felicity::date_now();
      seen.export_counters(output.seen, now);
      passed.export_counters(output.passed, now);
      adjusted.export_counters(output.adjusted, now);
      dropped.export_counters(output.dropped, now);
    }
  };

  template<typename T> void export_monotonicity_log(T& output, uint64_t passed_in_now = 0) {
    stats_.export_monotonicity_log(output, passed_in_now);
  }

 private:
  rolling_stats stats_;
  const uint64_t max_skew_;
  bool virgin_ = true;
  uint64_t last_ms_;
};

}  // namespace marvin
  
#endif
