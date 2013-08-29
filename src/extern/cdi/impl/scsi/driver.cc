/*
 * Copyright (c) 2009 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "cdi/scsi.h"
#include "cdi/printf.h"

int cdi_scsi_disk_init(struct cdi_scsi_device* device);

/**
 * Initialisiert die Datenstrukturen fuer einen SCSI-Treiber
 */
void cdi_scsi_driver_init(struct cdi_scsi_driver* driver)
{
    driver->drv.type = CDI_SCSI;
    cdi_driver_init((struct cdi_driver*) driver);
}

/**
 * Deinitialisiert die Datenstrukturen fuer einen SCSI-Treiber
 */
void cdi_scsi_driver_destroy(struct cdi_scsi_driver* driver)
{
    cdi_driver_destroy((struct cdi_driver*) driver);
}

/**
 * Initialisiert ein neues SCSI-Geraet
 *
 * Der Typ der Geraetes muss bereits gesetzt sein
 */
void cdi_scsi_device_init(struct cdi_scsi_device* device)
{
    switch (device->type) {
        case CDI_STORAGE:
            cdi_scsi_disk_init(device);
            break;

        default:
            CdiPrintf("No frontend for SCSI device of type %d\n",
                device->type);
            break;
    }
}

/**
 * Registiert den Treiber fuer SCSI-Geraete
 */
void cdi_scsi_driver_register(struct cdi_scsi_driver* driver)
{
    cdi_driver_register((struct cdi_driver*) driver);
}
