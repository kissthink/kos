/******************************************************************************
    Copyright 2012 Martin Karsten

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
#include "util/Log.h"
#include "dev/IODev.h"
#include "extern/stl/screenbuf.h"

static ScreenSegment top_screen( 1, 20, 2 );
static ScreenSegment bot_screen( 21, 25 );
static DebugDevice debugDevice;
static ScreenBuffer<char> topbuf(top_screen);
static ScreenBuffer<char> botbuf(bot_screen);
static ScreenBuffer<char> dbgbuf(debugDevice);

ostream kcout(&topbuf);
ostream kcerr(&botbuf);
ostream kcdbg(&dbgbuf);

void kassertprint(const char* const loc, int line, const char* const func) {
  kcerr << loc << line << " in " << func << " - ";
  kcdbg << loc << line << " in " << func << " - ";
}
