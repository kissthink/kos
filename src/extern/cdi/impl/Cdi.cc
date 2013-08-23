#include "cdi.h"

static cdi_list_t drivers = nullptr;
static cdi_list_t devices = nullptr;

void cdi_init() {
  drivers = cdi_list_create();
  devices = cdi_list_create();

  // populate devices list; OS should have probed for devices already
}

void cdi_driver_init(cdi_driver* driver) {
  driver->devices = cdi_list_create();
}

void cdi_driver_destroy(cdi_driver* driver) {
  cdi_list_destroy(driver->devices);
}

void cdi_driver_register(cdi_driver* driver) {
  cdi_list_push(drivers, driver);
}

#if 0
int cdi_provide_device(cdi_bus_data* device) {
  switch (device->bus_type) {
    cdi_pci_device* pci = reinterpret_cast<cdi_pci_device*>(device);
    if (pci->meta.initialized)  // already created
      return -1;

    // setup PCI device
    Device* device = new PCIDevice(pci->bus, pci->dev, pci_function,
        pci->vendor_id, pci->device_id,
        pci->class_id, pci->subclass_id, pci->interface_id,
        pci->rev_id, pci->irq);

    // set up I/O resources
    cdi_pci_resource* res = 0;
#endif
