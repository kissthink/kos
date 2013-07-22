#ifndef _inplace_allocator_h_
#define _inplace_allocator_h_

template<typename T,size_t shift> class InPlaceAllocator : public std::allocator<T> {
public:
  template<typename U> struct rebind { typedef InPlaceAllocator<U,shift> other; };
  InPlaceAllocator() = default;
  InPlaceAllocator(const InPlaceAllocator& x) = default;
  template<typename U> InPlaceAllocator (const InPlaceAllocator<U,shift>& x) : std::allocator<T>(x) {}
  ~InPlaceAllocator() = default;
  template<typename V>
  T* allocate(const V& val) {
		uintptr_t addr = (uintptr_t)(void*)(const_cast<V&>(val));
		addr = addr << shift;
		return reinterpret_cast<T*>(addr);
	}
  void deallocate(T*, size_t) {}
};

#endif /* _inplace_allocator_h_ */