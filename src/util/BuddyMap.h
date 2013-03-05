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
#ifndef _BuddyMap_h_
#define _BuddyMap_h_ 1

#include "util/Bitmask.h"

#include <map>

/* This implementation of a binary buddy system allocation uses an array of
 * STL sets to represent the free lists at each level. */

template<size_t min, size_t max, typename Set, typename DumpAllocator = typename Set::allocator_type>
class BuddyMap;

template<size_t min, size_t max, typename Set, typename DumpAllocator>
extern inline ostream& operator<<(ostream& os, const BuddyMap<min,max,Set,DumpAllocator>& bm);

template<size_t min, size_t max, typename Set, typename DumpAllocator>
class BuddyMap {
  typedef typename Set::iterator iterator;
  Set buddyLevel[max];
  Bitmask bitmask;

  friend ostream& operator<< <>( ostream&, const BuddyMap& );

private:
  void removeDirect( iterator it, size_t logsize ) {
    buddyLevel[logsize].erase( it );
    bitmask.clear(logsize, buddyLevel[logsize].empty());
  }

  void insertDirect( vaddr addr, size_t logsize ) {
    buddyLevel[logsize].insert(addr);
    bitmask.set(logsize);
  }

  void addToLevel( iterator it, vaddr addr, size_t logsize ) {
    buddyLevel[logsize].insert(it, addr);
  }

  // insert region and merge memory regions, if possible
  size_t insertAuto( vaddr addr, size_t logsize ) {
    KASSERT( logsize >= min && logsize < max, logsize );
    KASSERT( aligned(addr, pow2(logsize)), (ptr_t)addr );
    for (;;) {
      // set least relevant bit to zero to search for buddy
      vaddr search = addr & ~pow2(logsize);
      iterator it = buddyLevel[logsize].lower_bound( search );

      if ( it == buddyLevel[logsize].end() ) {
        // empty set or all regions at lower addresses -> insert and return
        insertDirect( addr, logsize );
        return logsize;
      } else if ( *it == addr ) {
        // region already present
        return max;
      }

      // if at last level (max - 1), don't merge
      if ( (logsize < max - 1) && (*it == (addr ^ pow2(logsize))) ) {
        // found buddy, remove (merge), then re-start search in next level
        removeDirect( it, logsize );
        logsize += 1;
        addr = search;
      } else {
        // buddy not found -> insert (possibly merged) region and return
        // 'lower_bound' finds equal or next / 'insert' suggests insert after
        // bitmask is already set
        addToLevel( --it, addr, logsize );
        return logsize;
      }
    }
  }

  // allocate region and reinsert leftover memory, if necessary
  size_t removeAuto( vaddr addr, size_t logsize ) {
    KASSERT( logsize >= min && logsize < max, logsize );
    KASSERT( aligned(addr, pow2(logsize)), (ptr_t)addr );
    // go through all levels to find region that covers request
    for ( size_t idx = logsize; ; idx += 1 ) {
      idx = bitmask.lsbc( idx, max );
      if ( idx >= max ) return max;
      vaddr search = addr & ~maskbits(idx);
      iterator it = buddyLevel[idx].find(search);
      if ( it != buddyLevel[idx].end() ) {
        removeDirect( it, idx );
        // reinsert leftover regions -> no merging possible in this case
        size_t prerange = addr - search;
        if ( prerange > 0 ) reinsert( search, prerange );
        size_t postrange = search + pow2(idx) - (addr + pow2(logsize));
        if ( postrange > 0 ) reinsert( addr + pow2(logsize), postrange );
        return idx;
      }
    }
  }

  // test, if region is available
  size_t testAuto( vaddr addr, size_t logsize ) {
    KASSERT( logsize >= min && logsize < max, logsize );
    KASSERT( aligned(addr, pow2(logsize)), (ptr_t)addr );
    for ( size_t idx = logsize; ; idx += 1 ) {
      idx = bitmask.lsbc( idx, max );
      if ( idx >= max ) return max;
      vaddr search = addr & ~maskbits(idx);
      iterator it = buddyLevel[idx].find(search);
      if ( it != buddyLevel[idx].end() ) return idx;
    }
  }

  struct InsertAuto {
    inline bool operator()( BuddyMap& bm, vaddr addr, size_t logsize ) {
      return bm.insertAuto( addr, logsize ) < max;
    }
  };

  struct RemoveAuto {
    inline bool operator()( BuddyMap& bm, vaddr addr, size_t logsize ) {
      return bm.removeAuto( addr, logsize ) < max;
    }
  };

  struct TestAuto {
    inline bool operator()( BuddyMap& bm, vaddr addr, size_t logsize ) {
      return bm.testAuto( addr, logsize ) < max;
    }
  };

  // break region into power2-sized chunks and perfrom requested action
  template<typename Action> bool region( vaddr start, size_t length ) {
    Action action;
    bool retcode = true;
    while ( length > 0 ) {
      KASSERT( aligned(start, pow2(min)), (ptr_t)start );
      KASSERT( aligned(length, pow2(min)), (ptr_t)length );
      size_t logsize = std::min( bitalignment(start), floorlog2(length) );
      retcode = action( *this, start, logsize ) && retcode;
      start += pow2(logsize);
      length -= pow2(logsize);
    }
    return retcode;
  }

  struct InsertDirect {
    inline bool operator()( BuddyMap& bm, vaddr addr, size_t logsize ) {
      bm.insertDirect( addr, logsize );
      return true;
    }
  };

  bool reinsert( vaddr start, size_t length ) {
    return region<InsertDirect>(start, length);
  }

public:
  bool remove( vaddr start, size_t length ) {
    return region<RemoveAuto>(start, length);
  }

  bool insert( vaddr start, size_t length ) {
    return region<InsertAuto>(start, length);
  }

  bool test( vaddr start, size_t length ) {
    return region<TestAuto>(start, length);
  }

  size_t insert2( vaddr start, size_t logsize ) {
    return insertAuto( start, logsize );
  }

  template<bool limit = false>
  vaddr retrieve( size_t length, vaddr upperlimit = mwordlimit ) {
    size_t logsize = ceilinglog2( length );
    KASSERT( logsize >= min && logsize < max, length );

    for (;;) {
      // find closest region with available space
      size_t index = bitmask.lsbc( logsize, max );
      if unlikely(index == max) return illaddr;
      KASSERT( index < max, index );

      // pick first available region
      KASSERT( !buddyLevel[index].empty(), index );
      iterator it = buddyLevel[index].begin();
      vaddr addr = *it;

      // test for upper limit
      if (!limit || addr < upperlimit ) {
        // remove region from map
        removeDirect( it, index );
        // reinsert leftover regions -> no merging possible in this case
        reinsert( addr + length, pow2(index) - length );
        return addr;
      }
      logsize = index + 1;
    }
  }
};

template<size_t min, size_t max, typename Set, typename DumpAllocator>
extern inline ostream& operator<<(ostream& os, const BuddyMap<min,max,Set,DumpAllocator>& bm) {
  std::map<vaddr,size_t,std::less<vaddr>,DumpAllocator> dumpMap;
  // store all memory regions in single dumpMap, sorted by start
  for ( size_t idx = 0; idx < max; ++idx ) {
    KASSERT( bm.bitmask.test(idx) == !bm.buddyLevel[idx].empty(), idx );
    for ( vaddr b : bm.buddyLevel[idx] ) dumpMap.insert( {b, pow2(idx)} );
  }
  // print continuous regions in compact representation
  vaddr end = mwordlimit;
  for ( const std::pair<vaddr,size_t>& d : dumpMap ) {
    if ( d.first != end ) {
      if ( end != mwordlimit ) os << (ptr_t)end << ' ';
      os << ptr_t(d.first) << '-';
    }
    end = d.first + d.second;
  }
  os << (ptr_t)end;
  return os;
}

#endif /* _BuddyMap_h_ */
