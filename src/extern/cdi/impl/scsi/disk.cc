/*
 * Copyright (C) 2008 Mathias Gottschlag
 * Copyright (C) 2009 Kevin Wolf
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "cdi/lists.h"
#include "cdi/scsi.h"
#include "cdi/storage.h"
#include "cdi/printf.h"

#define big_endian_word(x) ((((x) & 0xFF) << 8) | (((x) & 0xFF00) >> 8))
#define big_endian_dword(x) \
    ((big_endian_word((x) & 0xFFFF) << 16) | \
    (big_endian_word((x) >> 16)))

static int cdi_scsi_disk_read(struct cdi_storage_device* device,
    uint64_t start, uint64_t count, void* buffer);

static int cdi_scsi_disk_write(struct cdi_storage_device* device,
    uint64_t start, uint64_t count, void* buffer);

struct cdi_storage_driver cdi_scsi_disk_driver; //= {
#if 0
    .drv = {
        .type       = CDI_STORAGE,
    },
    .read_blocks    = cdi_scsi_disk_read,
    .write_blocks   = cdi_scsi_disk_write,
};
#endif

// TODO: This is kind of incomplete... -.-
// FIXME Das ist alles sehr CD-ROM-spezifisch hartkodiert...

union cdrom_command
{
    struct
    {
        uint8_t opcode;
        uint8_t reserved0;
        uint32_t address;
        uint8_t reserved1;
        uint16_t length;
        uint8_t reserved2;
    } __attribute__ ((packed)) dfl;
    struct
    {
        uint8_t opcode;
        uint8_t reserved0;
        uint32_t address;
        uint32_t length;
        uint16_t reserved1;
    } __attribute__ ((packed)) ext;
};

static int cdrom_read_sector(
    struct cdi_scsi_device *device, uint32_t sector, char *buffer)
{
    struct cdi_scsi_packet packet;
    struct cdi_scsi_driver* drv = (struct cdi_scsi_driver*) device->dev.driver;
    union cdrom_command* cmd = (union cdrom_command*) &packet.command;

    memset(&packet, 0, sizeof(packet));
    packet.direction = cdi_scsi_packet::CDI_SCSI_READ;
    packet.buffer = buffer;
    packet.bufsize = 2048;
    packet.cmdsize = 12;

    cmd->ext.opcode = 0xA8;
    cmd->ext.address = big_endian_dword(sector);
    cmd->ext.length = 0x01000000;

    if (drv->request(device, &packet)) {
        return -1;
    }

    return 0;
}

static int cdrom_capacity(struct cdi_scsi_device *device,
    uint32_t* num_sectors, uint32_t* sector_size)
{
    uint32_t buffer[32];
    struct cdi_scsi_packet packet;
    struct cdi_scsi_driver* drv = (struct cdi_scsi_driver*) device->dev.driver;
    union cdrom_command* cmd = (union cdrom_command*) &packet.command;

    memset(&packet, 0, sizeof(packet));
    packet.direction = cdi_scsi_packet::CDI_SCSI_READ;
    packet.buffer = buffer;
    packet.bufsize = sizeof(buffer);
    packet.cmdsize = 12;

    cmd->ext.opcode = 0x25;

    if (drv->request(device, &packet)) {
        return -1;
    }

    *num_sectors = big_endian_dword(buffer[0]);
    *sector_size = big_endian_dword(buffer[1]);

    return 0;
}

#if 0
static int cdrom_lock_device(FsSCSIDevice *device, int index, int lock)
{
    // Lock drive
    FsSCSIPacket packet;
    memset(&packet, 0, sizeof(FsSCSIPacket));
    packet.direction = CDI_SCSI_NO_DATA;
    ((union cdrom_command*)packet.command)->dfl.opcode = 0x1E;
    packet.command[4] = 1;
    packet.buffer = 0;
    packet.length = 0;
    packet.commandsize = 12;
    uint32_t status = device->request(device, &packet);
    if (status)
    {
        kePrint("cdrom %d: Lock failed, %x\n", index, status);
        fsCloseSCSIDevice(device);
        return KE_ERROR_UNKNOWN;
    }
    return 0;
}

static void cdrom_sense(FsSCSIDevice *device, uint32_t index)
{
    char sense_data[18];
    FsSCSIPacket packet;
    packet.direction = CDI_SCSI_READ;
    packet.buffer = sense_data;
    packet.length = 18;
    packet.commandsize = 12;
    ((union cdrom_command*)packet.command)->dfl.opcode = 0x12;
}
#endif

int cdrom_init(struct cdi_scsi_device* device)
{
    // Send inquiry command
    struct cdi_scsi_driver* drv = (struct cdi_scsi_driver*) device->dev.driver;
    struct cdi_scsi_packet packet;
    uint32_t status;
    unsigned char inqdata[96];

    memset(&packet, 0, sizeof(packet));
    memset(inqdata, 0xF0, sizeof(inqdata));

    packet.direction = cdi_scsi_packet::CDI_SCSI_READ;
    packet.buffer = inqdata;
    packet.bufsize = 96;
    packet.cmdsize = 12;

    ((union cdrom_command*)packet.command)->dfl.opcode = 0x12;
    packet.command[4] = 96;
    status = drv->request(device, &packet);
    if (status)
    {
        return -1;
    }

#if 0
    if (cdrom_lock_device(device, index, 1))
        return KE_ERROR_UNKNOWN;
#endif

    // Start drive
    packet.direction = cdi_scsi_packet::CDI_SCSI_NODATA;
    ((union cdrom_command*)packet.command)->dfl.opcode = 0x1B;
    packet.buffer = 0;
    packet.bufsize = 0;
    packet.command[4] = 0;
    packet.command[4] = 3;
    packet.cmdsize = 12;

    status = drv->request(device, &packet);
    if (status)
    {
        CdiPrintf("cdrom: Start failed, %x\n", status);
        return -1;
    }

    return 0;
}

static int cdi_scsi_disk_read(struct cdi_storage_device* device,
    uint64_t start, uint64_t count, void* buffer)
{
    struct cdi_scsi_device* dev = (cdi_scsi_device *) device->dev.backdev;

    while (count--) {
        if (cdrom_read_sector(dev, start, (char *) buffer)) {
            return -1;
        }
        start ++;
        buffer = (char *) buffer + 2048;
    }

    return 0;
}

static int cdi_scsi_disk_write(struct cdi_storage_device* device,
    uint64_t start, uint64_t count, void* buffer)
{
    return -1;
}

int cdi_scsi_disk_init(struct cdi_scsi_device* device)
{
    struct cdi_storage_device* frontdev;
    uint32_t num_sectors = 0, sector_size = 0;

    if (cdrom_init(device)) {
        return -1;
    }

    // initialize cdi_scsi_disk_driver here
    cdi_scsi_disk_driver.drv.type = CDI_STORAGE;
    cdi_scsi_disk_driver.read_blocks = cdi_scsi_disk_read;
    cdi_scsi_disk_driver.write_blocks = cdi_scsi_disk_write;

    frontdev = (cdi_storage_device *) malloc(sizeof(*frontdev));
    memset(frontdev, 0, sizeof(*frontdev));
    frontdev->dev.driver = (struct cdi_driver*) &cdi_scsi_disk_driver;
    frontdev->dev.name = device->dev.name;

    // Groesse des Mediums bestimmen. Falls keins eingelegt ist, brauchen wir
    // trotzdem zumindest eine Sektorgroesse.
    cdrom_capacity(device, &num_sectors, &sector_size);
    frontdev->block_size = sector_size ? sector_size : 2048;
    frontdev->block_count = num_sectors;

    frontdev->dev.backdev = device;
    device->frontdev = frontdev;

    // LostIO-Verzeichnisknoten anlegen
    cdi_storage_device_init(frontdev);

    return 0;
}
