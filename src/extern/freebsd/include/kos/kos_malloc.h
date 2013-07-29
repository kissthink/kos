#ifndef _KOS_MALLOC_h_
#define _KOS_MALLOC_h_

#ifdef __cplusplus
extern "C" {
#endif
  void *kos_malloc(unsigned long size);
  void kos_free(void *addr);
  unsigned long kos_getMemorySize(void *addr);
#ifdef __cplusplus
}
#endif

#endif /* _KOS_MALLOC_h_ */
