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

typedef void* ptr_t;
typedef const void* cptr_t;

typedef void (*function_t)(ptr_t);
typedef void (*funcvoid_t)();

static const uint32_t uint32_limit = uint32_t(~0);
static const size_t size_limit = size_t(~0);

template <typename T>
static inline constexpr T pow2( T x ) {
  return T(1) << x;
}

template <typename T>
static inline constexpr T maskbits( T b ) {
  return pow2<T>(b) - 1;
}

template <typename T>
static inline constexpr T maskbits( T b, T start, T end ) {
  return b & maskbits<T>(end) & ~maskbits<T>(start);
}

template <typename T>
static inline constexpr T align_up( T x, T a ) {
  return (x + a - 1) & (~(a - 1));  
}

template <typename T>
static inline constexpr T bitalign_up( T x, T b ) {
  return align_up<T>( x, pow2(b) );
}

template <typename T>
static inline constexpr T align_down( T x, T a ) {
  return x & (~(a - 1));  
}

template <typename T>
static inline constexpr T bitalign_down( T x, T b ) {
  return align_down<T>( x, pow2(b) );
}

template <typename T>
static inline constexpr bool aligned( T x, T a ) {
  return (x & (a - 1)) == 0;  
}

template <typename T>
static inline constexpr bool bitaligned( T x, T b ) {
  return aligned<T>( x, pow2(b) );
}

template <typename T, unsigned int position, unsigned int width> struct BitSeg  {
  static_assert( position + width <= 8*sizeof(T), "illegal position/width parameters" );
  constexpr T operator() (T f = maskbits<T>(width)) volatile const {
    return (f & maskbits<T>(width)) << position;
  }
  constexpr T get(T f) volatile const { 
    return (f >> position) & maskbits<T>(width);
  }
};

#endif /* _basics_h_ */
