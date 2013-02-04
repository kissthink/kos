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
#include "mach/PageManager.h"

const BitSeg<uint64_t, 0, 1> PageManager::P;
const BitSeg<uint64_t, 1, 1> PageManager::RW;
const BitSeg<uint64_t, 2, 1> PageManager::US;
const BitSeg<uint64_t, 3, 1> PageManager::PWT;
const BitSeg<uint64_t, 4, 1> PageManager::PCD;
const BitSeg<uint64_t, 5, 1> PageManager::A;
const BitSeg<uint64_t, 6, 1> PageManager::D;
const BitSeg<uint64_t, 7, 1> PageManager::PS;
const BitSeg<uint64_t, 8, 1> PageManager::G;  
const BitSeg<uint64_t,12,40> PageManager::ADDR;
const BitSeg<uint64_t,63, 1> PageManager::XD;
                      