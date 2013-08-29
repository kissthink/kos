#include "cdi/lists.h"

// KOS
#include "kern/Kernel.h"
#include "util/basics.h"
#include <list>
using namespace std;

// Use std list as a background implementation for CDI list
typedef list<ptr_t,KernelAllocator<ptr_t>> PtrList;

struct cdi_list_implementation {
  PtrList* _list;
};

cdi_list_t cdi_list_create() {
  cdi_list_implementation* impl = new cdi_list_implementation;
  impl->_list = new PtrList;
  return impl;
}

void cdi_list_destroy(cdi_list_t list) {
  delete list->_list; // delete std list
  delete list;        // delete wrapper implementation
}

cdi_list_t cdi_list_push(cdi_list_t list, ptr_t value) {
  list->_list->push_front(value); // insert to head of list
  return list;
}

ptr_t cdi_list_pop(cdi_list_t list) {
  if (list->_list->empty()) return nullptr;
  ptr_t elem = list->_list->front();
  list->_list->pop_front();
  return elem;
}

size_t cdi_list_empty(cdi_list_t list) {
  return list->_list->empty() ? 1 : 0;
}

ptr_t cdi_list_get(cdi_list_t list, size_t index) {
  auto it = list->_list->begin();
  size_t i = 0;
  while ( it != list->_list->end() ) {
    if (i == index) return *it;
    ++i, ++it;
  }
  return nullptr;
}

cdi_list_t cdi_list_insert(cdi_list_t list, size_t index, ptr_t value) {
  auto it = list->_list->begin();
  size_t i = 0;
  while ( it != list->_list->end() ) {
    if (i == index) {
      list->_list->insert(it, value);
      return list;
    }
    ++i, ++it;
  }
  return nullptr;
}

ptr_t cdi_list_remove(cdi_list_t list, size_t index) {
  auto it = list->_list->begin();
  size_t i = 0;
  while ( it != list->_list->end() ) {
    if ( i == index ) {
      ptr_t elem = *it;
      list->_list->erase( it );
      return elem;
    }
    ++i, ++it;
  }
  return nullptr;
}

size_t cdi_list_size(cdi_list_t list) {
  return list->_list->size();
}
