#ifndef _Calibrate_h_
#define _Calibrate_h_ 1

#include "mach/platform.h"

// from Linux's init/calibrate.c
class Calibrate {
  static mword cycles_per_tick;
  Calibrate() = delete;
  Calibrate(const Calibrate &c) = delete;
  Calibrate& operator=(const Calibrate& c) = delete;
  static const int MAX_DIRECT_CALIBRATION_RETRIES = 5;
  static const int DELAY_CALIBRATION_TICKS = 10;
public:
  static void init();
  static mword getCyclesPerTick() {
    return cycles_per_tick;
  }
};

#endif /* _Calibrate_h_ */

