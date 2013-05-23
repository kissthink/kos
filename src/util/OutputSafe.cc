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
#include "util/OutputSafe.h"
#include "dev/Screen.h"
#include "dev/Serial.h"

class ScreenBuffer : public OutputDevice {
  ScreenSegment segment;
public:
  ScreenBuffer(int firstline, int lastline, int startline = -1)
  : segment(firstline, lastline, startline) {}
  virtual streamsize write(const char* s, streamsize n) {
    for (streamsize i = 0; i < n; i += 1) segment.write(s[i]);
    return n;
  }
};

class DebugBuffer : public OutputDevice {
public:
  virtual streamsize write(const char* s, streamsize n) {
    for (streamsize i = 0; i < n; i += 1) DebugDevice::write(s[i]);
    return n;
  }
};

static ScreenBuffer top_screen( 1, 20, 2 );
static ScreenBuffer bot_screen( 21, 25 );
static DebugBuffer  dbg_buffer;

static OutputBuffer<char> topbuf(top_screen);
static OutputBuffer<char> botbuf(bot_screen);
static OutputBuffer<char> dbgbuf(dbg_buffer);

KernelOutput::KernelOutput( OutputBuffer<char>& ob ) : os(&ob) {}

KernelOutput StdOut(topbuf);
KernelOutput StdErr(botbuf);
KernelOutput StdDbg(dbgbuf);

void kassertprint(const char* const loc, int line, const char* const func) {
  StdErr.out(loc, line, " in ", func, " - ");
  StdDbg.out(loc, line, " in ", func, " - ");
}

void kassertprint1(const char* const msg) {
  StdErr.out(msg);
  StdDbg.out(msg);
}

void kassertprint1(const unsigned long long num) {
  StdErr.out(num);
  StdDbg.out(num);
}

void kassertprint1(const FmtHex& ptr) {
  StdErr.out(ptr);
  StdDbg.out(ptr);
}
