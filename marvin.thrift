// TODO(dkorolev): Auto-generate this file based on time_window_widths.h

struct Counters {
  1: i32 _,
  2: i32 _1m,
  3: i32 _3m,
  4: i32 _10m,
  5: i32 _30m,
  6: i32 _1h,
  7: i32 _4h,
  8: i32 _1d,
  9: i32 _1w,
}

struct LogMonotonicity {
  1: Counters seen,
  2: Counters passed,
  3: Counters adjusted,
  4: Counters dropped,
}
