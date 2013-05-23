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
#include "Debug.h"

#include <cstring>

static const char* options[] = {
  "acpi",
  "boot",
  "basic",
  "libc",
  "devices",
  "error",
  "frame",
  "gdballstop",
  "gdbdebug",
  "gdbenable",
  "memacpi",
  "paging",
  "pci",
  "scheduler",
  "vm",
  "warning",
};

Bitmask DBG::levels;               // stored in .bss, initialized early enough!

static_assert( sizeof(options)/sizeof(char*) == DBG::MaxLevel, "debug options mismatch" );

void DBG::init( char* dstring, bool msg ) {
  levels.set(Basic);
  char* wordstart = dstring;
  char* end = wordstart + strlen( dstring );
  for (;;) {
    char* wordend = strchr( wordstart, ',' );
    if ( wordend == nullptr ) wordend = end;
    *wordend = 0;
    size_t level = -1;
    for ( size_t i = 0; i < MaxLevel; ++i ) {
      if ( !strncmp(wordstart,options[i],wordend - wordstart) ) {
        if ( level == size_t(-1) ) level = i;
        else {
          if (msg) StdErr.outln("multi-match for debug option: ", wordstart);
          goto nextoption;
        }
      }
    }
    if ( level != size_t(-1) ) {
      if (msg) StdDbg.outln("matched debug option: ", wordstart, '=', options[level]);
      levels.set(level);
    } else {
      if (msg) StdErr.outln("unknown debug option: ", wordstart);
    }
nextoption:
  if ( wordend == end ) break;
    *wordend = ',';
    wordstart = wordend + 1;
  }
}
