/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "cdi/storage.h"
#include "cdi/misc.h"

#include "device.h"

#define DRIVER_NAME "floppy"

static struct cdi_storage_driver floppy_driver;

static struct floppy_controller floppy_controller = {
    // Standardmaessig DMA benutzen
    .use_dma = 1
};

/**
 * Initialisiert die Datenstrukturen fuer den floppy-Treiber
 */
static int floppy_driver_init(void)
{
    int i;
    struct floppy_device* device;

    // Konstruktor der Vaterklasse
    cdi_storage_driver_init(&floppy_driver);
    
    // Geraete erstellen (TODO: Was wenn eines oder beide nicht vorhanden
    // sind?)
    for (i = 0; i < 2; i++) {
        device = malloc(sizeof(*device));   
        device->id = i;

        // Die Nummer eines Floppylaufwerks ist immer einstellig
        device->dev.dev.name = malloc(4);
        snprintf((char*) device->dev.dev.name, 4, "fd%d", device->id);
        device->controller = &floppy_controller;

        // Geraet nur eintragen wenn es existiert
        if (floppy_device_probe(device) != 0) {
            device->dev.dev.driver = &floppy_driver.drv;
            cdi_list_push(floppy_driver.drv.devices, device);
        } else {
            free(device);
        }
    }

    // IRQ fuer FDC registrieren(ist hier egal fuer welches Geraet, da er eh
    // nur in der Controller-Struktur etwas macht).
    cdi_register_irq(6, floppy_handle_interrupt, (struct cdi_device*)
        cdi_list_get(floppy_driver.drv.devices, 0));

    // Kontroller initialisieren, falls dabei ein Fehler auftritt kann der
    // Treiber gleich wieder beendet werden
    if (floppy_init_controller(&floppy_controller) != 0) {
        return -1;
    }

    for (i = 0; (device = cdi_list_get(floppy_driver.drv.devices, i)); i++) {
        floppy_init_device(&device->dev.dev);
    }

    return 0;
}

/**
 * Deinitialisiert die Datenstrukturen fuer den sis900-Treiber
 */
static int floppy_driver_destroy(void)
{
    cdi_storage_driver_destroy(&floppy_driver);

    // TODO Alle Laufwerke deinitialisieren

    return 0;
}


static struct cdi_storage_driver floppy_driver = {
    .drv = {
        .type           = CDI_STORAGE,
        .name           = DRIVER_NAME,
        .init           = floppy_driver_init,
        .destroy        = floppy_driver_destroy,
        .remove_device  = floppy_remove_device,
    },
    .read_blocks        = floppy_read_blocks,
    .write_blocks       = floppy_write_blocks,
};

CDI_DRIVER(DRIVER_NAME, floppy_driver)
