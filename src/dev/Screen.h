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
#ifndef _Screen_h_
#define _Screen_h_ 1

#include "util/basics.h"
#include "mach/platform.h"

#include <cstring>

class Screen {
	friend class Machine;
private:
	static const int xmax = 160;
	static const int ymax = 25;
	static char  buffer[xmax * ymax] __aligned(0x1000);
	static char* video;

	static void init(mword displacement) {
		// monochrome screen would be at 0xb0000, increment by 1 byte
		if (((*(uint16_t*)0x410) & 0x30) == 0x30) Reboot();
		video = (char*)(displacement + 0xb8000);
		memcpy(buffer, video, ymax * xmax);
	}

	static vaddr getAddress() { return vaddr(video); }
	static void setAddress( vaddr vma ) { video = (char*)vma; }

	static void scroll( int offset, int length ) {
		memmove(buffer + offset, buffer + offset + xmax, length - xmax);
		memset(buffer + offset + length - xmax, 0, xmax );
		memcpy(video + offset, buffer + offset, length );
	}

	static void setcursor( int position ) {
		position = position / 2;
		out8(0x3D4, 14);
		out8(0x3D5, position >> 8);
		out8(0x3D4, 15);
		out8(0x3D5, position);
	}

public:
	Screen() = delete;                         // no creation
	Screen(const Screen&) = delete;            // no copy
	Screen& operator=(const Screen&) = delete; // no assignment

	static int offset( int firstline ) {
		return (firstline - 1) * xmax;
	}

	static int length( int firstline, int lastline ) {
		return (1 + lastline - firstline) * xmax;
	}

	static void cls( int offset, int length ) {
		memset(buffer + offset, 0, length);
		memcpy(video + offset, buffer + offset, length );
	}

	static void putchar( char c, int& position, int offset, int length ) {
		if (c == '\n') {
			position = ((position / xmax) + 1) * xmax;
		} else {
			buffer[position] = c;
			video[position] = c;
			position += 1;
			buffer[position] = 0x07;
			video[position] = 0x07;
			position += 1;
		}
		if (position >= offset + length) {
			scroll(offset, length);
			position -= xmax;
		}
		setcursor(position);
	}

};

class ScreenSegment {
  int offset;
  int length;
  int position;
public:
  ScreenSegment(int firstline, int lastline, int startline = -1)
    : offset(Screen::offset(firstline)),
      length(Screen::length(firstline,lastline)) {
    position = (startline > firstline) ? Screen::offset(startline) : Screen::offset(firstline);
  }
  ScreenSegment(const ScreenSegment&) = delete;            // no copy
  ScreenSegment& operator=(const ScreenSegment&) = delete; // no assignment
  void cls() { Screen::cls( offset, length ); }
  void write(char c) {
    Screen::putchar( c, position, offset, length );
  }
};

#endif /* _Screen_h_ */
