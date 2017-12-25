#pragma once

#include <cstdio>
#include <map>

#include "Debug.hpp"

struct Timer {
  using time_t = double;

  static constexpr int CURRENT_TIME = INT_MAX;

  time_t prev_time = .0;
  time_t current_time = .0;
  std::map<int, time_t> events;
  std::map<int, time_t> timeouts;

  Timer()
  {}

  /* void dump_times() { */
  /*   for(auto &[k,t] : events){Logger::Info("%d: %f\n",k,t);} */
  /* } */

  void set_time(time_t curtime) {
    prev_time = current_time;
    current_time = curtime;
  }

  void set_event(int key) {
    ASSERT(key != CURRENT_TIME);
    events[key] = current_time;
  }

  time_t elapsed(int key=CURRENT_TIME) const {
    /* Logger::Info("elapsed access: %d\n", key); */
    if(key == CURRENT_TIME)return current_time - prev_time;
    return current_time - events.at(key);
  }

  void set_timeout(int key, time_t timeout) {
    if(events.find(key) == events.end()) {
      set_event(key);
    }
    timeouts[key] = timeout;
  }

  bool timed_out(int key) const {
    /* Logger::Info("timeout access: %d\n", key); */
    time_t timeout = 0.;
    if(timeouts.find(key) != timeouts.end()) {
      timeout = timeouts.at(key);
    }
    return elapsed(key) > timeout;
  }
};