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

#include "serial_cdi.h"
#include "serial.h"
#include "cdi/lists.h"
#include "cdi/misc.h"

#include <stdio.h>
#include <string.h>

int serial_fs_init(struct cdi_fs_filesystem* cdi_fs)
{
    struct serial_fs_res* root_res;
    struct serial_fs_res* com1_res;
    struct serial_fs_res* com2_res;
    struct serial_fs_res* com3_res;
    struct serial_fs_res* com4_res;

    // Das FS initialisieren.

    root_res = malloc(sizeof(*root_res));
    memset(root_res, 0, sizeof(*root_res));
    root_res->res.name = strdup("/");
    root_res->res.res = &serial_fs_res;
    root_res->res.dir = &serial_fs_dir;
    root_res->res.loaded = 1;
    root_res->res.children = cdi_list_create();
    root_res->baseport = 0;

    com1_res = malloc(sizeof(*root_res));
    memset(com1_res, 0, sizeof(*root_res));
    com1_res->res.name = strdup("1");
    com1_res->res.res = &serial_fs_res;
    com1_res->res.dir = &serial_fs_dir;
    com1_res->res.loaded = 1;
    com1_res->res.children = cdi_list_create();
    com1_res->baseport = COM1_BASE;
    com1_res->in_use = 0;
    com1_res->res.parent = (struct cdi_fs_res*) root_res;
    cdi_list_push(root_res->res.children, com1_res);

    com2_res = malloc(sizeof(*root_res));
    memset(com2_res, 0, sizeof(*root_res));
    com2_res->res.name = strdup("2");
    com2_res->res.res = &serial_fs_res;
    com2_res->res.dir = &serial_fs_dir;
    com2_res->res.loaded = 1;
    com2_res->res.children = cdi_list_create();
    com2_res->baseport = COM2_BASE;
    com2_res->in_use = 0;
    com2_res->res.parent = (struct cdi_fs_res*) root_res;
    cdi_list_push(root_res->res.children, com2_res);

    com3_res = malloc(sizeof(*root_res));
    memset(com3_res, 0, sizeof(*root_res));
    com3_res->res.name = strdup("3");
    com3_res->res.res = &serial_fs_res;
    com3_res->res.dir = &serial_fs_dir;
    com3_res->res.loaded = 1;
    com3_res->res.children = cdi_list_create();
    com3_res->baseport = COM3_BASE;
    com3_res->in_use = 0;
    com3_res->res.parent = (struct cdi_fs_res*) root_res;
    cdi_list_push(root_res->res.children, com3_res);

    com4_res = malloc(sizeof(*root_res));
    memset(com4_res, 0, sizeof(*root_res));
    com4_res->res.name = strdup("4");
    com4_res->res.res = &serial_fs_res;
    com4_res->res.dir = &serial_fs_dir;
    com4_res->res.loaded = 1;
    com4_res->res.children = cdi_list_create();
    com4_res->baseport = COM4_BASE;
    com4_res->in_use = 0;
    com4_res->res.parent = (struct cdi_fs_res*) root_res;
    cdi_list_push(root_res->res.children, com4_res);

    cdi_fs->root_res = (struct cdi_fs_res*) root_res;
    return 1;
}

int serial_fs_destroy(struct cdi_fs_filesystem* fs)
{
    return serial_fs_res_destroy((struct serial_fs_res*)fs->root_res);
}
