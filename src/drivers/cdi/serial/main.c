/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Alexander Siol.
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

#include "cdi/fs.h"
#include "cdi/misc.h"

#include "serial_cdi.h"

#define DRIVER_NAME "serial"

static struct cdi_fs_driver serial_driver;

/**
 * Initialisiert die Datenstrukturen fuer den serial-Treiber
 */
static int serial_driver_init(void)
{
    // Konstruktor der Vaterklasse
    cdi_fs_driver_init(&serial_driver);

    return 0;
}

/**
 * Deinitialisiert die Datenstrukturen fuer den serial-Treiber
 */
static int serial_driver_destroy(void)
{
    cdi_fs_driver_destroy(&serial_driver);
    return 0;
}


static struct cdi_fs_driver serial_driver = {
    .drv = {
        .type       = CDI_FILESYSTEM,
        .name       = DRIVER_NAME,
        .init       = serial_driver_init,
        .destroy    = serial_driver_destroy,
    },
    .fs_init        = serial_fs_init,
    .fs_destroy     = serial_fs_destroy,
};

CDI_DRIVER(DRIVER_NAME, serial_driver)
