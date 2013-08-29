/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#include "cdi.h"
#include "cdi/pci.h"
#include "cdi/printf.h"

// KOS
#include "mach/Device.h"
#include "mach/PCI.h"

static cdi_list_t drivers = nullptr;            // global list of drivers

extern struct cdi_driver* __start_cdi_drivers;  // first driver
extern struct cdi_driver* __stop_cdi_drivers;   // 1 past last driver
static void cdi_run_drivers();                  // match device/driver pairs
static void cdi_init_pci_devices();             // PCI devices are detected

void cdi_init(void* arg) {
  drivers = cdi_list_create();

  // initialize and register each driver
  cdi_driver** pdrv;
  cdi_driver* drv;
  pdrv = &__start_cdi_drivers;
  while (pdrv < &__stop_cdi_drivers) {  // initialize and register drivers
    drv = *pdrv;
    if (drv->init != nullptr) {
      drv->init();
      cdi_driver_register(drv);         // insert to drivers list
    }
    pdrv += 1;
  }
  cdi_run_drivers();                    // match device/driver pairs
}

void cdi_driver_init(cdi_driver* driver) {
  driver->devices = cdi_list_create();  // create device list for the driver
}

void cdi_driver_destroy(cdi_driver* driver) {
  cdi_list_destroy(driver->devices);    // delete device list of the driver
}

void cdi_driver_register(cdi_driver* driver) {
  cdi_list_push(drivers, driver);       // insert to global drivers list
}

static void cdi_run_drivers() {
  cdi_init_pci_devices();               // detect PCI devices and assign drivers
  cdi_driver* driver;
  cdi_device* device;
  for (int i = 0; (driver = (cdi_driver *)cdi_list_get(drivers, i)); i++) {
    for (int j = 0; (device = (cdi_device *)cdi_list_get(driver->devices, j)); j++) {
      if (device->driver) {
        CdiPrintf("Device %s already has a driver %s assigned\n", device->name, driver->name);
      } else {
        device->driver = driver;    // match best driver for each device  (including non-PCI)
        CdiPrintf("Device %s -> Driver %s\n", device->name, driver->name);
      }
    }
  }
}

static void cdi_init_pci_devices() {
  cdi_driver* driver;
  cdi_device* device;
  cdi_pci_device* pci;

  cdi_list_t pci_devices = cdi_list_create();         // will store pci devices here
  cdi_pci_get_all_devices(pci_devices);               // populate the list with detected PCI devices

  for (int i = 0; (pci = (cdi_pci_device *)cdi_list_get(pci_devices, i)); i++) {
    // turn on I/O ports, MMIO, and bus mastering
    uint32_t val = PCI::readConfigWord(pci->bus, pci->dev, pci->function, 1, 32);
    PCI::writeConfigWord(pci->bus, pci->dev, pci->function, 1, 32, val | 0x7);

    // match best driver for each PCI device
    device = nullptr;
    for (int j = 0; (driver = (cdi_driver *)cdi_list_get(drivers, j)); j++) {
      if (driver->bus == CDI_PCI && driver->init_device) {
        device = driver->init_device(&pci->bus_data); // call device initialization code
        if (device) {
          device->driver = driver;                    // found a match, assign driver to the device
          break;
        }
      }
    }

    // PCI device has a suitable driver
    if (device != nullptr) {
      cdi_list_push(driver->devices, device);         // insert to devices list of the driver
      CdiPrintf("PCI device %04d:%04d:%04d - %s found!\n", pci->bus, pci->dev, pci->function, device->name);
    } else {
      cdi_pci_device_destroy(pci);
    }
  }

  cdi_list_destroy(pci_devices);    // delete pci device list
}
