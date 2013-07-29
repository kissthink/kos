#include "sys/uio.h"
#include "kos/kos_kassert.h"

struct uio *cloneuio(struct uio *uiop) {
  kos_reboot();
  return 0;
}

int copyinfrom(const void * __restrict src, void * __restrict dst, size_t len, int seg) {
  kos_reboot();
  return 0;
}

int copyiniov(struct iovec *iovp, u_int iovcnt, struct iovec **iov, int error) {
  kos_reboot();
  return 0;
}

int copyinstrfrom(const void * __restrict src, void * __restrict dst, size_t len,
                  size_t * __restrict copied, int seg) {
  kos_reboot();
  return 0;
}

int copyinuio(struct iovec *iovp, u_int iovcnt, struct uio **uiop) {
  kos_reboot();
  return 0;
}

int copyout_map(struct thread *td, vm_offset_t *addr, size_t sz) {
  kos_reboot();
  return 0;
}

int copyout_unmap(struct thread *td, vm_offset_t addr, size_t sz) {
  kos_reboot();
  return 0;
}

int	uiomove(void *cp, int n, struct uio *uio) {
  kos_reboot();
  return 0;
}

int	uiomove_frombuf(void *buf, int buflen, struct uio *uio) {
  kos_reboot();
  return 0;
}

int	uiomove_fromphys(struct vm_page *ma[], vm_offset_t offset, int n,
	                   struct uio *uio) {
  kos_reboot();
  return 0;
}

int	uiomove_nofault(void *cp, int n, struct uio *uio) {
  kos_reboot();
  return 0;
}

int	uiomoveco(void *cp, int n, struct uio *uio, int disposable) {
  kos_reboot();
  return 0;
}

