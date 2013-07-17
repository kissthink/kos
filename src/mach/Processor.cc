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
#include "mach/Processor.h"
#include "kern/Debug.h"

void Processor::checkCapabilities(bool print) {
  KASSERT0(RFlags::hasCPUID()); if (print) DBG::out(DBG::Basic, " CPUID");
  KASSERT0(CPUID::hasAPIC());   if (print) DBG::out(DBG::Basic, " APIC");
  KASSERT0(CPUID::hasMSR());    if (print) DBG::out(DBG::Basic, " MSR");
  KASSERT0(CPUID::hasNX());     if (print) DBG::out(DBG::Basic, " NX");
  if (CPUID::hasMWAIT())        if (print) DBG::out(DBG::Basic, " MWAIT");
  if (CPUID::hasARAT())         if (print) DBG::out(DBG::Basic, " ARAT");
  if (CPUID::hasTSC_Deadline()) if (print) DBG::out(DBG::Basic, " TSC");
  if (CPUID::hasX2APIC())       if (print) DBG::out(DBG::Basic, " X2A");
  if (print) DBG::out(DBG::Basic, kendl);
}
