#ifndef _C_Memory_h_
#define _C_Memory_h_

#ifdef __cplusplus
#include <cstddef>
typedef unsigned gfp_t;
extern "C" {
#endif

  void *kmalloc(size_t size, gfp_t flags);
  void *kzalloc(size_t size, gfp_t flags);

#ifdef __cplusplus
}
#endif

#endif /* C_Memory_h_ */
