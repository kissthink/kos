#include "mach/Calibrate.h"
#include "kern/Debug.h"
#include "mach/CPU.h"
#include "mach/Machine.h"

mword Calibrate::cycles_per_tick;
void Calibrate::init() {
  mword pre_start, start, post_start;   // pre_start is before tick increases, post_start is after tick increases
  mword pre_end, end, post_end;
  mword start_ticks;
  mword timer_rate_max, timer_rate_min;
  mword good_timer_sum = 0, good_timer_count = 0;
  mword measured_times[MAX_DIRECT_CALIBRATION_RETRIES];
  int max = -1, min = -1;
  for (int i = 0; i < MAX_DIRECT_CALIBRATION_RETRIES; i++) {
    pre_start = 0;
    start = CPU::readTSC();
    start_ticks = Machine::now();
    while (Machine::now() < start_ticks + 1) {
      pre_start = start;
      start = CPU::readTSC();
    }
    post_start = CPU::readTSC();

    pre_end = 0;
    end = post_start;
    while (Machine::now() < start_ticks + 1 + DELAY_CALIBRATION_TICKS) {
      pre_end = end;
      end = CPU::readTSC();
    }
    post_end = CPU::readTSC();

    timer_rate_max = (post_end - pre_start)/DELAY_CALIBRATION_TICKS;
    timer_rate_min = (pre_end - post_start)/DELAY_CALIBRATION_TICKS;

    if (start >= post_end)
      DBG::outln(DBG::Boot, "ignoring timer_rate as we had a TSC wrap around");
    if (start < post_end && pre_start != 0 && pre_end != 0
      && (timer_rate_max - timer_rate_min) < (timer_rate_max >> 3)) {
      good_timer_count++;
      good_timer_sum += timer_rate_max;
      measured_times[i] = timer_rate_max;
      if (max < 0 || timer_rate_max > measured_times[max]) max = i;
      if (min < 0 || timer_rate_min < measured_times[min]) min = i;
    } else {
      measured_times[i] = 0;
    }
    DBG::outln(DBG::Boot, "measured times: ", measured_times[i], " cycles/tick");
  }

  while (good_timer_count > 1) {
    mword estimate, maxdiff;
    estimate = good_timer_sum/good_timer_count;
    maxdiff = estimate >> 3;

    if ((measured_times[max] - measured_times[min]) < maxdiff) {
      DBG::outln(DBG::Boot, estimate, " cycles/tick");
      cycles_per_tick = estimate;
      return;
    }

    // drop the worse value and try again
    good_timer_sum = 0;
    good_timer_count = 0;
    if ((measured_times[max] - estimate) < (estimate - measured_times[min])) {
      measured_times[min] = 0;
      min = max;
    } else {
      measured_times[max] = 0;
      max = min;
    }

    for (int i = 0; i < MAX_DIRECT_CALIBRATION_RETRIES; i++) {
      if (measured_times[i] == 0) continue;
      good_timer_count++;
      good_timer_sum += measured_times[i];
      if (measured_times[i] < measured_times[min]) min = i;
      if (measured_times[i] > measured_times[max]) max = i;
    }
  }
}

