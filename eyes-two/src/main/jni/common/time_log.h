// Copyright 2010 Google Inc. All Rights Reserved.
// Author: andrewharp@google.com (Andrew Harp)
//
// Utility functions for performance profiling.

#ifndef JAVA_COM_GOOGLE_ANDROID_APPS_UNVEIL_JNI_COMMON_TIME_LOG_H_
#define JAVA_COM_GOOGLE_ANDROID_APPS_UNVEIL_JNI_COMMON_TIME_LOG_H_

#include <time.h>

#include "utils.h"
#include "types.h"

#ifdef LOG_TIME

inline static long currentThreadTimeNanos() {
  struct timespec tm;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tm);
  return tm.tv_sec * 1000000000LL + tm.tv_nsec;
}

// Blend constant for running average.
#define ALPHA 0.98f
#define NUM_LOGS 100

struct LogEntry {
  const char* id;
  clock_t time_stamp;
};

struct AverageEntry {
  const char* id;
  float32 average_duration;
};

// Storage for keeping track of this frame's values.
extern int32 num_time_logs;
extern LogEntry time_logs[NUM_LOGS];

// Storage for keeping track of average values (each entry may not be printed
// out each frame).
extern AverageEntry avg_entries[NUM_LOGS];
extern int32 num_avg_entries;
extern float32 running_total;

// Call this at the start of a logging phase.
inline static void resetTimeLog() {
  num_time_logs = 0;
}


// Log a message to be printed out when printTimeLog is called, along with the
// amount of time in ms that has passed since the last call to this function.
inline static void timeLog(const char* str) {
  if (num_time_logs >= NUM_LOGS) {
    LOGE("Out of log entries!");
    return;
  }

  time_logs[num_time_logs].id = str;
  time_logs[num_time_logs].time_stamp = currentThreadTimeNanos();
  ++num_time_logs;
}


inline static float32 blend(float32 old_val, float32 new_val) {
  return ALPHA * old_val + (1.0f - ALPHA) * new_val;
}


inline static float32 updateAverage(const char* str, const float32 new_val) {
  for (int32 entry_num = 0; entry_num < num_avg_entries; ++entry_num) {
    AverageEntry* const entry = avg_entries + entry_num;
    if (str == entry->id) {
      entry->average_duration = blend(entry->average_duration, new_val);
      return entry->average_duration;
    }
  }

  if (num_avg_entries >= NUM_LOGS) {
    LOGE("Too many log entries!");
  }

  // If it wasn't there already, add it.
  avg_entries[num_avg_entries].id = str;
  avg_entries[num_avg_entries].average_duration = new_val;
  ++num_avg_entries;

  return new_val;
}


// Prints out all the timeLog statements in chronological order with the
// interval that passed between subsequent statements.  The total time between
// the first and last statements is printed last.
inline static void printTimeLog() {
  LogEntry* last_time = time_logs;

  for (int i = 0; i < num_time_logs; ++i) {
    LogEntry* this_time = time_logs + i;

    const float32 curr_time =
        (this_time->time_stamp - last_time->time_stamp) / 1000000.0f;

    const float32 avg_time = updateAverage(this_time->id, curr_time);
    LOGD("%32s:    %6.2fms    %6.2fms", this_time->id, curr_time, avg_time);
    last_time = this_time;
  }

  float32 total_time =
      (last_time->time_stamp - time_logs->time_stamp) / 1000000.0f;

  running_total = blend(running_total, total_time);

  LOGD("TOTAL TIME:                          %6.2fms    %6.2fms\n",
       total_time, running_total);
}
#else
inline static void resetTimeLog() {}
inline static void timeLog(const char* str) {}
inline static void printTimeLog() {}
#endif

#endif  // JAVA_COM_GOOGLE_ANDROID_APPS_UNVEIL_JNI_COMMON_TIME_LOG_H_
