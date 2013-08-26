#include "cdi.h"
#include "cdi/pci.h"

// KOS
#include "mach/Device.h"
#include "mach/PCI.h"

static cdi_list_t drivers = nullptr;

extern struct cdi_driver* __start_cdi_drivers;
extern struct cdi_driver* __stop_cdi_drivers;

static void cdi_run_drivers();
static void cdi_init_pci_devices();

void cdi_init() {
  drivers = cdi_list_create();

  // initialize and register all of active drivers
  cdi_driver** pdrv;
  cdi_driver* drv;
  pdrv = &__start_cdi_drivers;
  while (pdrv < &__stop_cdi_drivers) {
    drv = *pdrv;
    if (drv->init != nullptr) {
      drv->init();
      cdi_driver_register(drv);
    }
    pdrv += 1;
  }

  // run all drivers
  cdi_run_drivers();
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

static void cdi_run_drivers() {
  // initialize PCI devices
  cdi_init_pci_devices();

  cdi_driver* driver;
  cdi_device* device;
  for (int i = 0; (driver = (cdi_driver *)cdi_list_get(drivers, i)); i++) {
    for (int j = 0; (device = (cdi_device *)cdi_list_get(driver->devices, j)); j++) {
      device->driver = driver;
      // TODO init net device
    }
    // register service
  }
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

static void cdi_init_pci_devices() {
  cdi_driver* driver;
  cdi_device* device;
  cdi_pci_device* pci;

  cdi_list_t pci_devices = cdi_list_create();
  cdi_pci_get_all_devices(pci_devices);

  for (int i = 0; (pci = (cdi_pci_device *)cdi_list_get(pci_devices, i)); i++) {
    // Enable I/O ports, MMIO and Bus-mastering
    uint32_t val = PCI::readConfigWord(pci->bus, pci->dev, pci->function, 1, 32);
    PCI::writeConfigWord(pci->bus, pci->dev, pci->function, 1, 32, val | 0x7);

    device = nullptr;
    for (int j = 0; (driver = (cdi_driver *)cdi_list_get(drivers, j)); j++) {
      if (driver->bus == CDI_PCI && driver->init_device) {
        device = driver->init_device(&pci->bus_data);
        if (device) {
          device->driver = driver;
          break;
        }
      }
    }

    if (device != nullptr) {
      cdi_list_push(driver->devices, device);
      DBG::outln(DBG::CDI, "CDI: ", FmtHex(pci->bus), '.',
          FmtHex(pci->dev), '.', FmtHex(pci->function), ": ",
          driver->name);
    } else {
      cdi_pci_device_destroy(pci);
    }
  }

  cdi_list_destroy(pci_devices);
}
