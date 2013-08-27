/*
 * Copyright (c) 2007-2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>

#include "cdi.h"
#include "cdi/storage.h"
#include "cdi/misc.h"
#include "cdi/io.h"

#include "device.h"

/**
 * ATA-Device identification
 *
 * @return 0 when the unit has been successfully identified, != 0 otherwise
 */
int ata_drv_identify(struct ata_device* dev)
{
    struct ata_identfiy_data id;
    // prepare request
    struct ata_request request = {
        .dev = dev,

        .flags.direction = READ,
        .flags.poll = 1,
        .flags.lba = 0,

        // identification data is read over PIO DATA IN
        .protocol = PIO,
        .registers.ata.command = IDENTIFY_DEVICE,
        .block_count = 1,
        .block_size = ATA_SECTOR_SIZE,
        .buffer = &id,

        .error = 0
    };
    
    // Request start
    if (!ata_request(&request)) {
        // if an error has occurred, we can still try it with a
        // IDENTIFY PACKET device
        return atapi_drv_identify(dev);
    }
    
    // check LBA mode
    if (id.features_support.bits.lba48) {
        dev->lba48 = 1;
        dev->dev.storage.block_count = id.max_lba48_address;
    }
    if (id.capabilities.lba) {
        dev->lba28 = 1;
        dev->dev.storage.block_count = id.lba_sector_count;
    }

    // check if DMA supported
    if (id.capabilities.dma) {
        dev->dma = 1;
    }

    // if none of the LBA mode is supported, must be canceled because
    // CHS is not yet implemented
    if (!dev->lba48 && !dev->lba28) {
        DEBUG_ATA("Device supports CHS only.\n");
        return 0;
    }

    // an ata device
    dev->atapi = 0;

    return 1;
}

/**
 * RW sectors from a ATA device
 *
 * @param direction 0 for read, 1 for write
 * @param start LBA of the starting sector
 * @param count number of sectors
 * @param buffer Pointer to buffer into which the data is to be stored, respective
 * from which they are to be read.
 *
 * @return 1 when the blocks were successfully read/written, 0 otherwise
 */
static int ata_drv_rw_sectors(struct ata_device* dev, int direction,
    uint64_t start, size_t count, void* buffer)
{
    int result = 1;
    struct ata_request request;
    // since not more than 256 sectors can be read at once or written, keep
    // current count
    uint16_t current_count;
    void* current_buffer = buffer;
    uint64_t lba = start;
    int max_count;
    int again = 2;
    // number of sectors that are remained
    size_t count_left = count;

    // prepare request
    request.dev = dev;
    request.flags.poll = 0;
    request.flags.ata = 0;
    request.flags.lba = 1;

    // determine the direction
    if (direction == 0) {
        request.flags.direction = READ;
    } else {
        request.flags.direction = WRITE;
    }


    if (dev->dma && dev->controller->dma_use) {
        // DMA
        max_count = ATA_DMA_MAXSIZE / ATA_SECTOR_SIZE;
        if (direction) {
            request.registers.ata.command = WRITE_SECTORS_DMA;
        } else {
            request.registers.ata.command = READ_SECTORS_DMA;
        }
        request.protocol = DMA;
    } else {
        // PIO
        max_count = 256;
        if (direction) {
            request.registers.ata.command = WRITE_SECTORS;
        } else {
            request.registers.ata.command = READ_SECTORS;
        }
        request.protocol = PIO;

        // prefer PIO poll since IRQs are limited??? (can't translate)
        request.flags.poll = 1;
    }

    // as long as sectors to be read remains
    while (count_left > 0) {
        // decide how many sectors are read in the current pass
        if (count_left > max_count) {
            current_count = max_count;
        } else {
            current_count = count_left;
        }
        
        // Achtung: Beim casten nach uint8_t wird bei 256 Sektoren eine 0.
        // Das macht aber nichts, da in der Spezifikation festgelegt ist,
        // dass 256 Sektoren gelesen werden sollen, wenn im count-Register
        // 0 steht.
        // Caution: when cast by a uint8_t is at 256 sectors 0.
        // But that doest not matter because is defined in the specification, that 256
        // sectors are to be read when standing in the count register 0.
        request.registers.ata.count = (uint8_t) current_count;
        request.registers.ata.lba = lba;

        request.block_count = current_count;
        request.block_size = ATA_SECTOR_SIZE;
        request.blocks_done = 0;
        request.buffer = current_buffer;

        request.error = NO_ERROR;
        
        // TODO: LBA48
        // TODO: CHS
        
        // execute request
        if (!ata_request(&request)) {
            if (again) {
                again--;
                continue;
            }
            result = 0;
            break;
        }

        // adjust buffer pointer an number of other blocks
        current_buffer += current_count * ATA_SECTOR_SIZE;
        count_left -= current_count;
        lba += current_count;
        again = 2;
    }

    return result;
}


/**
 * Read sectors from a ATA device
 *
 * @param start LBA of the starting sector
 * @param count number of sectors
 * @param buffer pointer to the buffer into which the data will be stored
 *
 * @return 1 if the block has been read successfully, 0 otherwise
 */
int ata_drv_read_sectors(struct ata_device* dev, uint64_t start, size_t count,
    void* buffer)
{
    return ata_drv_rw_sectors(dev, 0, start, count, buffer);
}

/**
 * Write sectors to a ATA device
 *
 * @param start LBA of the starting sector
 * @param count number of sectors
 * @param buffer Pointer to the buffer from which the data are to be read
 *
 * @return 1 when the blocks were successfully written, 0 otherwise.
 */
int ata_drv_write_sectors(struct ata_device* dev, uint64_t start, size_t count,
    void* buffer)
{
    return ata_drv_rw_sectors(dev, 1, start, count, buffer);
}

