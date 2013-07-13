/*
 * Based on drivers/base/dd.c
 *
 * Implements few exported functions for core device/driver interactions.
 */

#include "linux/device.h"
#include "base.h"

void *dev_get_drvdata(const struct device *dev) {
  if (dev && dev->p) return dev->p->driver_data;
  return NULL;
}

int dev_set_drvdata(struct device *dev, void *data) {
  int error;
  if (!dev->p) {
    error = device_private_init(dev);
    if (error) return error;
  }
  dev->p->driver_data = data;
  return 0;
}
