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
  "paging",
  "pci",
  "scheduler",
  "warning",
};

SpinLock DBG::lk;
Bitmask DBG::levels;

static_assert( sizeof(options)/sizeof(char*) == DBG::MaxLevel, "debug options mismatch" );

void DBG::init( char* dstring, bool msg ) {
  levels.init();
  levels.set(Basic);
  char* wordstart = dstring;
  char* end = wordstart + strlen( dstring );
  for (;;) {
    char* wordend = strchr( wordstart, ',' );
    if ( wordend == nullptr ) wordend = end;
    *wordend = 0;
    size_t level = size_limit;
    for ( size_t i = 0; i < MaxLevel; ++i ) {
      if ( !strncmp(wordstart,options[i],wordend - wordstart) ) {
        if ( level == size_limit ) level = i;
        else {
          if (msg) kcerr << "multi-match for debug option: " << wordstart << '\n';
          goto nextoption;
        }
      }
    }
    if ( level != size_limit ) {
      if (msg) kcdbg << "matched debug option: " << wordstart << '=' << options[level] << '\n';
      levels.set(level);
    } else {
      if (msg) kcerr << "unknown debug option: " << wordstart << '\n';
    }
nextoption:
  if ( wordend == end ) break;
    *wordend = ',';
    wordstart = wordend + 1;
  }
}
