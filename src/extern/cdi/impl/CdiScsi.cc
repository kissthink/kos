#include "cdi/scsi.h"
#include "cdi/storage.h"
#include "cdi/printf.h"

// KOS
#include "util/basics.h"
#include <cstring>
#include <cstdlib>

#define big_endian_word(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#define big_endian_dword(x) \
    ((big_endian_word((x) & 0xffff) << 16) | (big_endian_word((x) >> 16)))

int cdi_scsi_disk_init(cdi_scsi_device*);

void cdi_scsi_driver_init(cdi_scsi_driver* driver) {
  driver->drv.type = CDI_SCSI;
  cdi_driver_init((cdi_driver *) driver);
}

void cdi_scsi_driver_destroy(cdi_scsi_driver* driver) {
  cdi_driver_destroy((cdi_driver *) driver);
}

void cdi_scsi_device_init(cdi_scsi_device* device) {
  switch (device->type) {
    case CDI_STORAGE:
      cdi_scsi_disk_init(device);
      break;
    default:
      CdiPrintf("No frontend for SCSI device of type %d\n", device->type);
      break;
  }
}

void cdi_scsi_driver_register(cdi_scsi_driver* driver) {
  cdi_driver_register((cdi_driver *) driver);
}

union cdrom_command {
  struct {
    uint8_t opcode;
    uint8_t reserved0;
    uint32_t address;
    uint8_t reserved1;
    uint16_t length;
    uint8_t reserved2;
  } __packed dfl;
  struct {
    uint8_t opcode;
    uint8_t reserved0;
    uint32_t address;
    uint16_t reserved1;
  } __packed ext;
};

int cdrom_init(cdi_scsi_device* device) {
  cdi_scsi_driver* drv = (cdi_scsi_driver *) device->dev.driver;
  cdi_scsi_packet packet;
  uint32_t status;
  unsigned char inqdata[96];

  memset(&packet, 0, sizeof(packet));
  memset(inqdata, 0xf0, sizeof(inqdata));

  packet.direction = cdi_scsi_packet::CDI_SCSI_READ;
  packet.buffer = inqdata;
  packet.bufsize = 96;
  packet.cmdsize = 12;

  ((union cdrom_command *) packet.command)->dfl.opcode = 0x12;
  packet.command[4] = 96;
  status = drv->request(device, &packet);
  if (status) return -1;

  // start drive
  packet.direction = cdi_scsi_packet::CDI_SCSI_NODATA;
  ((union cdrom_command *)packet.command)->dfl.opcode = 0x18;
  packet.buffer = 0;
  packet.bufsize = 0;
  packet.command[4] = 0;
  packet.command[4] = 3;
  packet.cmdsize = 12;

  status = drv->request(device, &packet);
  if (status) {
    CdiPrintf("cdrom: start failed, %x\n", status);
    return -1;
  }
  return 0;
}

int cdrom_capacity(cdi_scsi_device* device, uint32_t* num_sectors, uint32_t* sector_size) {
  uint32_t buffer[32];
  cdi_scsi_packet packet;
  cdi_scsi_driver* drv = (cdi_scsi_driver *) device->dev.driver;
  union cdrom_command *cmd = (union cdrom_command *) &packet.command;

  memset(&packet, 0, sizeof(packet));
  packet.direction = cdi_scsi_packet::CDI_SCSI_READ;
  packet.buffer = buffer;
  packet.bufsize = sizeof(buffer);
  packet.cmdsize = 12;

  cmd->ext.opcode = 0x25;

  if (drv->request(device, &packet)) return -1;
  *num_sectors = big_endian_dword(buffer[0]);
  *sector_size = big_endian_dword(buffer[1]);

  return 0;
}

int cdi_scsi_disk_init(cdi_scsi_device* device) {
  cdi_storage_device* frontdev;
  uint32_t num_sectors = 0, sector_size = 0;

  if (cdrom_init(device)) return -1;

#if 0
  frontdev = (cdi_storage_device *) calloc(1, sizeof(*frontdev));
  frontdev->dev.driver = (cdi_driver *) &cdi_scsi_disk_driver;
  frontdev->dev.name = device->dev.name;

  cdrom_capacity(device, &num_sectors, &sector_size);
  frontdev->block_size = sector_size ? sector_size : 2048;
  frontdev->block_count = num_sectors;

  frontdev->dev.backdev = device;
  device->frontdev = frontdev;
#endif

  cdi_storage_device_init(frontdev);

  return 0;
}
