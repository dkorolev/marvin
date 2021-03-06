// Defines a data structure keeping and allowing access to user-computed statistics over a sliding [time] window.
//
// Implements:
// 
// * void add(const T::element_type);
//   Adds an entry and takes care of removing the ones that just went out of the desired time window.
//
// * bool empty() const, size_t size().
//   Basic and other simple getters.
// 
// * const T& stats(timestamp_type ms [= safe version of last_ms()]) const;
//   Returns a const reference to an underlying object keeping the statistics.
//   If necessary, relaxes the entries.
//   Single-threaded.
//
// Requires:
//
// * Entries coming in a non-decreasing order of their timestamps.
//   Would terminate with an error message should this happen.
//
// Implementation requirements:
//
// * Type T exists.
//
// * Type T::element_type exists.
//
// * The way to export a timestamp from an instance of T::element_type is provided, see ms_sliding_window_stats below.
//   In most cases it translates to having a 'uint64_t ms' field or its equivalent.
//
// * Type T exports on_enqueue(const T::element_type&) and on_dequeue(const T::element_type&) methods.
//   These methods are the ones keeping the user-defined statistics.

#ifndef SLIDING_WINDOW_STATS_H
#define SLIDING_WINDOW_STATS_H

#include "../felicity/safe_ostream.h"

#include <deque>

namespace marvin {

const uint64_t ONE_MINUTE_IN_MS = 1000 * 60;
const uint64_t ONE_HOUR_IN_MS = ONE_MINUTE_IN_MS * 60;
const uint64_t ONE_DAY_IN_MS = ONE_HOUR_IN_MS * 24;
const uint64_t ONE_WEEK_IN_MS = ONE_DAY_IN_MS * 7;

struct stats_accessor {
  template<typename T1, typename T2> inline static void enqueue(T1& a, const T2& b) {
    a.on_enqueue(b);
  }
  template<typename T1, typename T2> inline static void dequeue(T1& a, const T2& b) {
    a.on_dequeue(b);
  }
};

template<typename element_type, typename timestamp_type>
typename std::enable_if<std::is_arithmetic<element_type>::value, timestamp_type>::type simple_timestamp_of(const element_type& entry) {
  return static_cast<timestamp_type>(entry);
}

template<typename element_type, typename timestamp_type>
typename std::enable_if<!std::is_arithmetic<element_type>::value, timestamp_type>::type simple_timestamp_of(const element_type& entry) {
  return static_cast<timestamp_type>(entry.ms);
}

struct timestamp_of_wrapper {
  template<typename element_type, typename timestamp_type>  static inline timestamp_type timestamp_of(const element_type& entry) {
    return simple_timestamp_of<element_type, timestamp_type>(entry);
  }
};

template<
  typename T_STATS,
  typename T_TIMESTAMP = uint64_t,
  T_TIMESTAMP DEFAULT_WINDOW = ONE_WEEK_IN_MS,
  typename T_TIMESTAMP_WRAPPER = timestamp_of_wrapper,
  typename T_QUEUE = std::deque<typename T_STATS::element_type>>
class sliding_window_stats {

public:
  typedef T_STATS stats_type;
  typedef typename T_STATS::element_type element_type;
  typedef T_TIMESTAMP timestamp_type;

  sliding_window_stats() {
  }

  sliding_window_stats(const timestamp_type& window_width) : window_width_(window_width) {
  }

  sliding_window_stats(const sliding_window_stats&) = delete;
  void operator=(const sliding_window_stats&) = delete;

  void add(const element_type& e) {
    relax(T_TIMESTAMP_WRAPPER::template timestamp_of<element_type, timestamp_type>(e));
    entries_.push_back(e), stats_accessor::enqueue(stats_, entries_.back());
  }

  const stats_type& stats() const {
    return stats_;
  }

  const stats_type& stats(const timestamp_type timestamp) {
    relax(timestamp);
    return stats_;
  }

  const bool empty() const {
    return entries_.empty();
  }

  const size_t size() const {
    return entries_.size();
  }

  const timestamp_type window_width() const {
    return window_width_;
  }

  void relax(timestamp_type timestamp) {
    if (virgin_) {
      virgin_ = false;
    } else {
      if (timestamp < most_recent_timestamp_) {
        std::ostringstream os;
        os << "Time went back, from " << most_recent_timestamp_ << " to " << timestamp << ".\n";
        const std::string s = os.str();
        felicity::safe_cout << s;
        felicity::safe_cerr << s;
        exit(1);
      }
    }
    most_recent_timestamp_ = timestamp;
    while (!entries_.empty() && (most_recent_timestamp_ - T_TIMESTAMP_WRAPPER::template timestamp_of<element_type, timestamp_type>(entries_.front()) > window_width_)) {
      stats_accessor::dequeue(stats_, entries_.front()), entries_.pop_front();
    }
  }

 private:
  stats_type stats_;
  T_QUEUE entries_;
  const timestamp_type window_width_ = DEFAULT_WINDOW;
  bool virgin_ = true;
  timestamp_type most_recent_timestamp_;
};

// Class dummy_stats keeps no statistics.
// It is used as a placeholder to make it possible to call sliding_window_stats.size() on a stream of 'ms' values.
template<typename T> class dummy_stats {
 public:
  typedef T element_type;
 private:
  friend class stats_accessor;
  inline void on_enqueue(T x) {}
  inline void on_dequeue(T x) {}
};

// Class sliding_windows keeps track of multiple sliding windows of different widths at once.
template<typename T_STATS, typename T_TIMESTAMP = uint64_t> struct sliding_windows {
  size_t total = 0;
#define TIME_WINDOW_WIDTH(name,ms) sliding_window_stats<T_STATS, T_TIMESTAMP, ms> counters_##name;
#include "time_window_widths.h"
#undef TIME_WINDOW_WIDTH

  template<typename F> void for_each(F f) {
#define TIME_WINDOW_WIDTH(name,ms) f(counters_##name);
#include "time_window_widths.h"
#undef TIME_WINDOW_WIDTH
  }

  template<typename F> void for_each(F f) const {
#define TIME_WINDOW_WIDTH(name,ms) f(counters_##name);
#include "time_window_widths.h"
#undef TIME_WINDOW_WIDTH
  }

  template<typename F, typename X> void for_each(F f, X& x) {
#define TIME_WINDOW_WIDTH(name,ms) f(counters_##name, x._##name);
#include "time_window_widths.h"
#undef TIME_WINDOW_WIDTH
  }

  template<typename F, typename X> void for_each(F f, X& x) const {
#define TIME_WINDOW_WIDTH(name,ms) f(counters_##name, x._##name);
#include "time_window_widths.h"
#undef TIME_WINDOW_WIDTH
  }

  struct add_impl {
    const typename T_STATS::element_type& e_;
    add_impl(const typename T_STATS::element_type& e) : e_(e) {}
    template<typename T> inline void operator()(T& c) { c.add(e_); }
  };

  void add(const typename T_STATS::element_type& e) {
    ++total;
    for_each(add_impl(e));
  }

  struct relax_impl {
    const T_TIMESTAMP timestamp_;
    relax_impl(const T_TIMESTAMP timestamp) : timestamp_(timestamp) {}
    template<typename T> inline void operator()(T& c) { c.relax(timestamp_); }
  };

  void relax(T_TIMESTAMP timestamp) {
    for_each(relax_impl(timestamp));
  }

  struct export_counters_impl {
    template<typename T, typename X> inline void operator()(const T& c, X& r) {
      r = c.size();
    }
  };

  template<typename T_COUNTERS> void export_counters(T_COUNTERS& output) const {
    output._ = total;
    for_each(export_counters_impl(), output);
  }
  template<typename T_COUNTERS> void export_counters(T_COUNTERS& output, T_TIMESTAMP timestamp = 0) {
    if (timestamp) {
      relax(timestamp);
    }
    output._ = total;
    for_each(export_counters_impl(), output);
  }
};

template<typename T1, typename T2> void extract_first_n(T1& output, const T2& input, size_t size = 100) {
  if (input.size() <= size) {
    std::copy(input.begin(), input.end(), back_inserter(output));
  } else {
    size_t count = 0;
    for (auto cit = input.begin(); count < size; ++cit, ++count) {
      output.push_back(*cit);
    }
  }
}

template<typename T1, typename T2> void reverse_extract_last_n(T1& output, const T2& input, size_t size = 100) {
  if (input.size() <= size) {
    std::copy(input.rbegin(), input.rend(), back_inserter(output));
  } else {
    size_t count = 0;
    for (auto cit = input.rbegin(); count < size; ++cit, ++count) {
      output.push_back(*cit);
    }
  }
}

template<typename T1, typename T2> void copy_most_recent(T1& output, const T2& input, size_t size = 100) {
  if (input.size() <= size) {
    std::copy(input.begin(), input.end(), back_inserter(output));
  } else {
    std::copy(input.begin(), input.begin() + size, back_inserter(output));
  }
}

template<typename T> void reverse(T& x) {
  std::reverse(x.begin(), x.end());
}

template<typename T1, typename T2> void reverse_copy_most_recent(T1& output, const T2& input, size_t size = 100) {
  if (input.size() <= size) {
    std::copy(input.rbegin(), input.rend(), back_inserter(output));
  } else {
    std::copy(input.rbegin(), input.rbegin() + size, back_inserter(output));
  }
}

// Poor man's test.
// So far only tests that the code compiles.
// TODO(dkorolev): Add a proper test.

struct stats_element {
  uint64_t ms;
};

struct stats {
  typedef stats_element element_type;
  inline void on_enqueue(const element_type&) {
  }
  inline void on_dequeue(const element_type&) {
  }
};

struct should_compile_sliding_window_usage {
  template<typename T> inline void test_by_type() {
    T impl;
    typename T::element_type e;
    impl.add(e);
    impl.empty();
    impl.size();
    impl.window_width();
    const typename T::stats_type& s = impl.stats();
  }
  inline void test() {
    test_by_type<sliding_window_stats<stats>>();
    test_by_type<sliding_window_stats<stats, uint64_t, ONE_MINUTE_IN_MS>>();
    test_by_type<sliding_window_stats<dummy_stats<uint64_t>>>();
    test_by_type<sliding_window_stats<dummy_stats<int64_t>>>();
    test_by_type<sliding_window_stats<dummy_stats<uint64_t>, uint64_t, ONE_MINUTE_IN_MS>>();
    test_by_type<sliding_window_stats<dummy_stats<int64_t>, uint64_t, ONE_HOUR_IN_MS>>();
    test_by_type<sliding_window_stats<dummy_stats<double>, uint64_t, ONE_WEEK_IN_MS>>();
  }
};

}  // namespace marvin

#endif
