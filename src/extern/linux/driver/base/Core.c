/*
 * Based on drivers/base/core.c
 *
 * Implements few exported functions for core driver model code (device registrations, etc)
 */

#include "linux/device.h"
#include "linux/klist.h"
#include "linux/gfp.h"
#include "base.h"
#include "Memory.h"

static void klist_children_get(struct klist_node *n) {
  struct device_private *p = to_device_private_parent(n);
  struct device *dev = p->device;

  get_device(dev);
}

static void klist_children_put(struct klist_node *n) {
  struct device_private *p = to_device_private_parent(n);
  struct device *dev = p->device;

  put_device(dev);
}

int device_private_init(struct device *dev) {
  dev->p = kzalloc(sizeof(*dev->p), GFP_KERNEL);
  if (!dev->p) return -ENOMEM;
  dev->p->device = dev;
  klist_init(&dev->p->klist_children, klist_children_get, klist_children_put);
  INIT_LIST_HEAD(&dev->p->deferred_probe);
  return 0;
}
