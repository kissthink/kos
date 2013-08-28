#include "cdi/storage.h"
#include "cdi/printf.h"

// KOS
#include "kern/Kernel.h"
#include "util/basics.h"
#include <map>
#include <cstring>

map<kstring,cdi_storage_device*,less<kstring>,KernelAllocator<pair<kstring,cdi_storage_device>>> storageDevices;

void cdi_storage_driver_init(cdi_storage_driver* driver) {
  driver->drv.type = CDI_STORAGE;
  cdi_driver_init((cdi_driver *) driver);
}

void cdi_storage_driver_destroy(cdi_storage_driver* driver) {
  cdi_driver_destroy((cdi_driver *) driver);
}

void cdi_storage_driver_register(cdi_storage_driver* driver) {
  cdi_driver_register((cdi_driver *) driver);
}

void cdi_storage_device_init(cdi_storage_device* device) {
  CdiPrintf("Storage device init: %s\n", device->dev.name);
  KASSERT0( storageDevices.count(device->dev.name) == 0 );
  storageDevices[device->dev.name] = device;
}

int cdi_storage_read(cdi_storage_device* device, uint64_t pos,
                     size_t size, ptr_t dest)
{
  cdi_storage_driver* driver = (cdi_storage_driver *) device->dev.driver;
  size_t block_size = device->block_size;
  uint64_t block_read_start = pos / block_size;
  uint64_t block_read_end = (pos + size) / block_size;
  uint64_t block_read_count = block_read_end - block_read_start;

  if (((pos % block_size) == 0) && (((pos + size) % block_size) == 0)) {
    return driver->read_blocks(device, block_read_start, block_read_count, dest);
  } else {
    block_read_count += 1;
    uint8_t* buffer = new uint8_t[block_read_count * block_size];
    if (driver->read_blocks(device, block_read_start, block_read_count, buffer) != 0) return -1;
    memcpy(dest, buffer + (pos % block_size), size);
    delete [] buffer;
  }
  return 0;
}

int cdi_storage_read(const kstring& devName, uint64_t pos, size_t size, ptr_t dest) {
  if (storageDevices.count(devName) == 0) {
    CdiPrintf("Storage device %s not found\n", devName.c_str());
    return -1;  // device not found
  }
  return cdi_storage_read(storageDevices[devName], pos, size, dest);
}

int cdi_storage_write(cdi_storage_device* device, uint64_t pos, size_t size, ptr_t src) {
  cdi_storage_driver* driver = (cdi_storage_driver *) device->dev.driver;
  size_t block_size = device->block_size;
  uint64_t block_write_start = pos / block_size;
  uint8_t* buffer = new uint8_t[block_size];
  size_t offset;
  size_t tmp_size;
  offset = (pos % block_size);
  if (offset != 0) {
    tmp_size = block_size - offset;
    tmp_size = (tmp_size > size ? size : tmp_size);
    if (driver->read_blocks(device, block_write_start, 1, buffer) != 0) return -1;
    memcpy(buffer + offset, src, tmp_size);
    if (driver->write_blocks(device, block_write_start, 1, buffer) != 0) return -1;
    size -= tmp_size;
    src = (ptr_t)((char *)src + tmp_size);
    block_write_start += 1;
  }

  tmp_size = size / block_size;
  if (tmp_size != 0) {
    if (driver->write_blocks(device, block_write_start, tmp_size, src) != 0) return -1;
    size -= tmp_size * block_size;
    src = (ptr_t)((char *)src + tmp_size * block_size);
    block_write_start += block_size;
  }
  if (size != 0) {
    if (driver->read_blocks(device, block_write_start, 1, buffer) != 0) return -1;
    memcpy(buffer, src, size);
    if (driver->write_blocks(device, block_write_start, 1, buffer) != 0) return -1;
  }
  return 0;
}

int cdi_storage_write(const kstring& devName, uint64_t pos, size_t size, ptr_t dest) {
  if (storageDevices.count(devName) == 0) {
    CdiPrintf("Storage device %s not found\n", devName.c_str());
    return -1;  // device not found
  }
  return cdi_storage_write(storageDevices[devName], pos, size, dest);
}
