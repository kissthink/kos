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
#ifndef _basics_h_
#define _basics_h_ 1

#include <cstddef>
#include <cstdint>

#if defined(likely) || defined(unlikely)
#error macro collision: 'likely' or 'unlikely'
#endif
#define likely(x)   (__builtin_expect((x),1))
#define unlikely(x) (__builtin_expect((x),0))

#define __aligned(x) __attribute__((__aligned__(x)))
#define __packed     __attribute__((__packed__))
#define __section(x) __attribute__((__section__(x)))
#define __finline    __attribute__((__always_inline__))

typedef void* ptr_t;
typedef const void* cptr_t;
typedef char  buf_t;
typedef char* bufptr_t;
typedef const char* cbufptr_t;

typedef void (*function_t)(ptr_t);
typedef void (*funcvoid_t)();

template <typename T>
static inline constexpr T pow2( T x ) {
  return T(1) << x;
}

template <typename T>
static inline constexpr T powN( T x, T y ) {
  return y == 0 ? 1 : powN(x,y-1);
}

template <typename T>
static inline constexpr T align_up( T x, T a ) {
  return (x + a - 1) & (~(a - 1));  
}

template <typename T>
static inline constexpr T align_down( T x, T a ) {
  return x & (~(a - 1));  
}

template <typename T>
static inline constexpr bool aligned( T x, T a ) {
  return (x & (a - 1)) == 0;  
}

template <typename T>
static inline constexpr T maskbits( T b ) {
  return pow2<T>(b) - 1;
}

template <typename T>
static inline constexpr T maskbits( T start, T end ) {
  return maskbits<T>(end) & ~maskbits<T>(start);
}

template <typename T>
static inline constexpr T divup( T n, T d ) {
  return 1 + (n - 1) / d;
}

template <typename T, unsigned int position, unsigned int width> struct BitSeg  {
  static_assert( position + width <= 8*sizeof(T), "illegal position/width parameters" );
  // create bitmask at right position
  constexpr T operator() () volatile const {
    return maskbits<T>(width) << position;
  }
  // create value at right position
  constexpr T operator() (T f) volatile const {
    return (f & maskbits<T>(width)) << position;
  }
  // obtain normalized value
  constexpr T get(T f) volatile const { 
    return (f >> position) & maskbits<T>(width);
  }
  // obtain value at right position
  constexpr T extract(T f) volatile const {
    return f & (maskbits<T>(width) << position);
  }
#if defined(__clang__)
  BitSeg() {}
#endif
};

#endif /* _basics_h_ */
