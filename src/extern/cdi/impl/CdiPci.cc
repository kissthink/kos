#include "cdi/pci.h"

// KOS
#include "mach/Device.h"
#include "mach/IOPort.h"
#include "mach/IOPortManager.h"
#include "mach/IOMemory.h"
#include "mach/IOMemoryManager.h"
#include "mach/PCI.h"
#include "mach/PCIDevice.h"

static void add_child_devices(cdi_list_t list, Device* dev) {
  for (unsigned int i = 0; i < dev->getNumChildren(); i++) {
    DBG::outln(DBG::CDI, "Device type: ", FmtHex(dev->getChild(i)->getType()));
    if (dev->getChild(i)->getType() == Device::PCI) {
      PCIDevice* child = (PCIDevice *)(dev->getChild(i));
      cdi_pci_device* pciDev = new cdi_pci_device;
      pciDev->bus_data.bus_type = CDI_PCI;
      pciDev->bus = child->getBus();
      pciDev->dev = child->getDevice();
      pciDev->function = child->getFunction();
      pciDev->vendor_id = child->getVendorID();
      pciDev->device_id = child->getDeviceID();
      pciDev->class_id = child->getClassCode();
      pciDev->subclass_id = child->getSubClass();
      pciDev->interface_id = child->getProgIF();
      pciDev->rev_id = child->getRevision();
      pciDev->irq = child->getInterruptNumber();
      pciDev->resources = cdi_list_create(); 

      // TODO should add backdev?

      // add BARs 
      uint8_t headerType = child->getHeaderType();
      uint8_t numBars;
      switch (headerType & 0x7f) {
        case 0x00:  // standard
          numBars = 6; break;
        case 0x01:  // PCI-PCI bridge
          numBars = 2; break;
        default:
          numBars = 0;
      }
      for (int i = 0; i < numBars; i++) {
        cdi_pci_resource* res = new cdi_pci_resource;
        uint32_t bar = child->getBAR(i);
        if (bar == 0) {
          delete res;
          continue;
        }
        res->type = (bar & 0x1 ? CDI_PCI_IOPORTS : CDI_PCI_MEMORY);
        if (res->type == CDI_PCI_IOPORTS) {
          res->start = (bar & 0xfffffffc);
          res->length = child->getBARSize(i);
          res->index = i;
          res->address = 0; // not yet mapped to vaddr
        } else {
          uint8_t memType = ((bar & 0x6) >> 1);
          if (memType == 0x00) {        // 32-bit wide
            res->start = (bar & 0xfffffff0);
            res->length = child->getBARSize(i);
            res->index = i;
            res->address = 0;
          } else if (memType == 0x01) { // reserved for PCI LocalBus 3.0
            ABORT1("Do not know how to interpret this BAR");
          } else if (memType == 0x02) { // 64-bit wide
            uint32_t bar2 = child->getBAR(i + 1);
            res->start = ((bar & 0xfffffff0) + (((uintptr_t)(bar2 & 0xffffffff)) << 32));
            res->length = (child->getBARSize(i) + (((uint64_t)child->getBARSize(i+1)) << 32));
            res->index = i;
            res->address = 0;
            i += 1;   // consumed 2 registers
          } else ABORT1("unknown type field");
        }
        cdi_list_push(pciDev->resources, res);
      }
      cdi_list_push(list, pciDev);
    }
    add_child_devices(list, dev->getChild(i));
  }
}

void cdi_pci_get_all_devices(cdi_list_t list) {
  Device* root = Device::getRoot();
  add_child_devices(list, root);
}

void cdi_pci_device_destroy(cdi_pci_device* device) {
  // TODO
}

void cdi_pci_alloc_ioports(cdi_pci_device* device) {
  cdi_pci_resource* res;
  for (int i = 0; (res = (cdi_pci_resource *)cdi_list_get(device->resources, i)); i++) {
    if (res->type == CDI_PCI_IOPORTS) {
//      IOPort ioPort("temp port");
      IOPort* ioPort = new IOPort("PCI port");
      bool success = IOPortManager::allocate(*ioPort, res->start, res->length);
      KASSERT0( success );
    }
  }
}

void cdi_pci_free_ioports(cdi_pci_device* device) {
  cdi_pci_resource* res;
  for (int i = 0; (res = (cdi_pci_resource *)cdi_list_get(device->resources, i)); i++) {
    if (res->type == CDI_PCI_IOPORTS) {
      KASSERT0( IOPortManager::release( res->start, res->length ) );
    }
  }
}

void cdi_pci_alloc_memory(cdi_pci_device* device) {
  cdi_pci_resource* res;
  for (int i = 0; (res = (cdi_pci_resource *)cdi_list_get(device->resources, i)); i++) {
    if (res->type == CDI_PCI_MEMORY) {
//      IOMemory mem("temp");
      IOMemory* mem = new IOMemory("PCI memory");
      bool success = IOMemoryManager::allocRegion(*mem, laddr(res->address), res->length, 0);
      KASSERT0( success );
    }
  }
}

void cdi_pci_free_memory(cdi_pci_device* device) {
  cdi_pci_resource* res;
  for (int i = 0; (res = (cdi_pci_resource *)cdi_list_get(device->resources, i)); i++) {
    if (res->type == CDI_PCI_MEMORY) {
      // TODO
      res->address = 0;
    }
  }
}

const char* cdi_pci_get_device_name(cdi_pci_device* device) {
  return PCI::getDeviceName(device->vendor_id, device->device_id);
}

uint8_t cdi_pci_config_readb(cdi_pci_device* device, uint8_t offset) {
  return PCI::readConfigWord(device->bus, device->dev, device->function, offset, 8);
}

uint16_t cdi_pci_config_readw(cdi_pci_device* device, uint8_t offset) {
  return PCI::readConfigWord(device->bus, device->dev, device->function, offset, 16);
}

uint32_t cdi_pci_config_readl(cdi_pci_device* device, uint8_t offset) {
  return PCI::readConfigWord(device->bus, device->dev, device->function, offset, 32);
}

void cdi_pci_config_writeb(cdi_pci_device* device, uint8_t offset, uint8_t value) {
  PCI::writeConfigWord(device->bus, device->dev, device->function, offset, 8, value);
}

void cdi_pci_config_writew(cdi_pci_device* device, uint8_t offset, uint16_t value) {
  PCI::writeConfigWord(device->bus, device->dev, device->function, offset, 16, value);
}

void cdi_pci_config_writel(cdi_pci_device* device, uint8_t offset, uint32_t value) {
  PCI::writeConfigWord(device->bus, device->dev, device->function, offset, 32, value);
}
