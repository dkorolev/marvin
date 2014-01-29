// TODO(dkorolev): Auto-generate this file based on time_window_widths.h

struct Counters {
  1: i32 total,
  2: i32 total_1m,
  3: i32 total_3m,
  4: i32 total_10m,
  5: i32 total_30m,
  6: i32 total_1h,
  7: i32 total_4h,
  8: i32 total_1d,
  9: i32 total_1w,
}

struct LogMonotonicity {
  1: Counters seen,
  2: Counters passed,
  3: Counters adjusted,
  4: Counters dropped,
}
