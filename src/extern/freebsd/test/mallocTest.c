#include "sys/param.h"
#include "sys/module.h"
#include "sys/kernel.h"
#include "sys/systm.h"
#include "sys/malloc.h"

#include "kos/Kassert.h"

// M_WAITOK with string
void mallocTest1() {
  char *str = malloc(sizeof(char) * 10, 0, M_WAITOK);
  strncpy(str, "malloc", 7);
  BSD_KASSERT0(strncmp(str, "malloc", 7) == 0);
  free(str, 0);
}
SYSINIT(mallocTest1, SI_SUB_DRIVERS, SI_ORDER_FIRST, mallocTest1, NULL);

// M_ZERO test
void mallocTest2() {
  int *num = malloc(sizeof(int) * 3, 0, M_WAITOK | M_ZERO);
  int i;
  for (i = 0; i < 3; i++) {
    BSD_KASSERT0(0 == num[i]);
  }
  free(num, 0);
}
SYSINIT(mallocTest2, SI_SUB_DRIVERS, SI_ORDER_FIRST, mallocTest2, NULL);

// M_WAITOK | M_NOWAIT
void mallocTest3() {
  int *num = malloc(sizeof(int), 0, M_WAITOK | M_NOWAIT);
  printf("mallocTest3: bad malloc should show: %d\n", num[0]);
}
//SYSINIT(mallocTest3, SI_SUB_DRIVERS, SI_ORDER_FIRST, mallocTest3, NULL);

// realloc
void mallocTest4() {
  char *str = realloc(NULL, sizeof(char) * 10, 0, M_WAITOK);
  strncpy(str, "realloc", 8);
  BSD_KASSERT0(strncmp(str, "realloc", 8) == 0);
  str = realloc(str, sizeof(char) * 20, 0, M_WAITOK);
  BSD_KASSERT0(strncmp(str, "realloc", 8) == 0);
  str = realloc(str, sizeof(char) * 3, 0, M_WAITOK);
  printf("string: %s\n", str);
  BSD_KASSERT0(strncmp(str, "realloc", 8) == 0);
  free(str, 0);
}
SYSINIT(mallocTest4, SI_SUB_DRIVERS, SI_ORDER_FIRST, mallocTest4, NULL);
