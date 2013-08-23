#include "cdi/net.h"
#include "cdi.h"

// KOS
#include "util/basics.h"

void cdi_net_driver_init(struct cdi_net_driver* driver) {
  driver->drv.type = CDI_NETWORK;
  cdi_driver_init(reinterpret_cast<cdi_driver*>(driver));
}

void cdi_net_driver_destroy(struct cdi_net_driver* driver) {
  cdi_driver_destroy(reinterpret_cast<cdi_driver*>(driver));
}

void cdi_net_device_init(struct cdi_net_device* device) {
  // TODO
}

void cdi_net_receive(cdi_net_device* device, ptr_t buffer, size_t size) {
  // TODO
}
