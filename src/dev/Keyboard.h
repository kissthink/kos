/******************************************************************************
    Copyright 2012-2013 Martin Karsten

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
// details taken from
// http://wiki.osdev.org/%228042%22_PS/2_Controller
// http://www.brokenthorn.com/Resources/OSDev19.html
// http://www.brokenthorn.com/Resources/Demos/Demo15/SysCore/Keyboard/kybrd.cpp

#ifndef _Keyboard_h_
#define _Keyboard_h_ 1

#include "kern/KernelQueues.h"
#include "kern/SyncQueues.h"

//#define KEYBOARD_RAW 1

class Keyboard {
public:
  typedef int KeyCode;
private:
  ProdConsQueue<StaticRingBuffer<KeyCode,128>> kbq;

  // state machine variables
  int  error;                           // set if keyboard error
  int  scancode;                        // current scan code
  bool is_break;                        // make or break code
  bool extended;                        // extended key code
  bool bat_res;                         // set if the Basic Assurance Test (BAT) failed
  bool diag_res;                        // set if diagnostics failed
  bool resend_res;                      // set if system should resend last command
  bool disabled;                        // set if keyboard is disabled
  bool shift, alt, ctrl;                // shift, alt, and ctrl keys
  uint8_t led;                          // led status byte
  int acks;                             // number of expected acks

  // controller/encoder interface
  inline uint8_t ctrl_read_status();
  inline bool read_buffer();
  inline bool write_buffer();
  inline void ctrl_send_cmd(uint8_t);
  inline uint8_t enc_read_buf();
  inline void enc_send_cmd(uint8_t);
  inline void drain_buffer();

  // resend last command
  inline void ignore_resend() { resend_res = false; }
  inline bool check_resend(){ return resend_res; }

  // returns status of tests. self_test runs the test
  bool get_diagnostic_res() { return diag_res; }
  bool get_bat_res() { return bat_res; }
  inline bool self_test();

  // updates LEDs
  inline void set_leds();

public:
  Keyboard() : error(0), scancode(0), is_break(false), extended(false),
    bat_res(true), diag_res(false), resend_res(false), disabled(false),
    shift(false), alt(false), ctrl(false), led(0), acks(0) {}
  void init()                                          __section(".boot.text");
  void staticInterruptHandler();
  KeyCode read() { return kbq.remove(); } // read keycode

  // returns status of lock keys
  bool get_scroll_lock() { return led & 1; }
  bool get_numlock()     { return led & 2; }
  bool get_capslock()    { return led & 4; }

  // returns status of special keys
  bool get_alt()         { return alt; }
  bool get_ctrl()        { return ctrl; }
  bool get_shift()       { return shift; }

  // keyboard enable / disable
  bool is_disabled()     { return disabled; }
  void disable();
  void enable();

  // reset system
  void reset_system();
};

#endif /* _Keyboard_h_ */
