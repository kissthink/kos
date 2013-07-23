#include "linux/spinlock_types.h"
#include "linux/irqflags.h"
#include "SpinLock.h"

// Due to extensive use of macros in Linux's spinlock_init() and DEFINE_SPINLOCK
// functions that are used to initialize spinlock_t dynamically/statically,
// I could not find a better way than lazily create SpinLock class, which is
// embedded in raw_spinlock_t struct. I used atomic CAS operation but not 100%
// sure this is the right approach
void initSpinLock(raw_spinlock_t *lock) {
  if (!__sync_val_compare_and_swap(&lock->lockImplInit, 0, 1))
    lock->lockImpl = SpinLockCreate();
}

// spin_lock()
void __raw_spin_lock(raw_spinlock_t *lock) {
  initSpinLock(lock);
  SpinLockAcquire(lock->lockImpl);
}
void _raw_spin_lock(raw_spinlock_t *lock) {
  __raw_spin_lock(lock);
}

// spin_lock_irqsave()
unsigned long __raw_spin_lock_irqsave(raw_spinlock_t *lock) {
  unsigned long flags;
  local_irq_save(flags);              // also disables interrupt
  initSpinLock(lock);
  SpinLockAcquire(lock->lockImpl);
  return flags;
}
unsigned long _raw_spin_lock_irqsave(raw_spinlock_t *lock) {
  return __raw_spin_lock_irqsave(lock);
}

// spin_lock_irq()
void __raw_spin_lock_irq(raw_spinlock_t *lock) {
  local_irq_disable();
  initSpinLock(lock);
  SpinLockAcquire(lock->lockImpl);
}
void _raw_spin_lock_irq(raw_spinlock_t *lock) {
  __raw_spin_lock_irq(lock);
}

// spin_lock_bh()
void __raw_spin_lock_bh(raw_spinlock_t *lock) {
//  local_bh_disable(); FIXME when bottom half is implemented
  initSpinLock(lock);
  SpinLockAcquire(lock->lockImpl);
}
void _raw_spin_lock_bh(raw_spinlock_t *lock) {
  __raw_spin_lock_bh(lock);
}

// spin_trylock()
int __raw_spin_trylock(raw_spinlock_t *lock) {
  initSpinLock(lock);
  return SpinLockTryAcquire(lock->lockImpl);
}
int _raw_spin_trylock(raw_spinlock_t *lock) {
  return __raw_spin_trylock(lock);
}

// spin_trylock_bh()
int __raw_spin_trylock_bh(raw_spinlock_t *lock) {
//  local_bh_disable(); FIXME when bottom half is implemented
  initSpinLock(lock);
  return SpinLockTryAcquire(lock->lockImpl);
}
int _raw_spin_trylock_bh(raw_spinlock_t *lock) {
  return __raw_spin_trylock_bh(lock);
}

// spin_unlock()
void __raw_spin_unlock(raw_spinlock_t *lock) {
  SpinLockRelease(lock->lockImpl);
}
void _raw_spin_unlock(raw_spinlock_t *lock) {
  __raw_spin_unlock(lock);
}

// spin_unlock_irqrestore()
void __raw_spin_unlock_irqrestore(raw_spinlock_t *lock, unsigned long flags) {
  SpinLockRelease(lock->lockImpl);
  local_irq_restore(flags);
}
void _raw_spin_unlock_irqrestore(raw_spinlock_t *lock, unsigned long flags) {
  __raw_spin_unlock_irqrestore(lock, flags);
}

// spin_unlock_irq()
void __raw_spin_unlock_irq(raw_spinlock_t *lock) {
  SpinLockRelease(lock->lockImpl);
  local_irq_enable();
}
void _raw_spin_unlock_irq(raw_spinlock_t *lock) {
  __raw_spin_unlock_irq(lock);
}

// spin_unlock_bh()
void __raw_spin_unlock_bh(raw_spinlock_t *lock) {
  SpinLockRelease(lock->lockImpl);
//  local_bh_enable_ip(); FIXME when bottom half is implemented
}
void _raw_spin_unlock_bh(raw_spinlock_t *lock) {
  __raw_spin_unlock_bh(lock);
}
