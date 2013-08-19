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

#include <stdio.h>
#include <string.h>

#include "serial_cdi.h"
#include "serial.h"

int serial_fs_res_load(struct cdi_fs_stream* stream)
{
    struct serial_fs_res* res = (struct serial_fs_res*) stream->res;

    if (res->res.loaded) {
        return 0;
    }

    res->res.loaded = 1;
    return 1;
}

int serial_fs_res_unload(struct cdi_fs_stream* stream)
{
    struct serial_fs_res* res = (struct serial_fs_res*) stream->res;

    if (!res->res.loaded) {
        return 0;
    }

    res->res.loaded = 0;
    return 1;
}

int64_t serial_fs_res_meta_read(struct cdi_fs_stream* stream, cdi_fs_meta_t meta)
{
    return 0;
}

int serial_fs_res_meta_write(struct cdi_fs_stream* stream, cdi_fs_meta_t meta,
                           int64_t value)
{
    return 0;
}

int serial_fs_res_assign_class(struct cdi_fs_stream* stream,
                             cdi_fs_res_class_t class)
{
    struct serial_fs_res* res = (struct serial_fs_res*) stream->res;
    struct serial_fs_res* parent = (struct serial_fs_res*) res->res.parent;


    if (class == CDI_FS_CLASS_FILE)
    {
        res->res.file = &serial_fs_file;

        // Namen auseinandernehmen
        char* trenner = NULL;
        uint32_t baudrate;
        parity_t parity   = PARITY_NONE;
        uint8_t bits     = 8;
        uint8_t stopbits = 1;

        trenner = index(res->res.name, ',');
        if (trenner)
        {
            char* baudrate_s = malloc(trenner - res->res.name + 1);
            if (!baudrate_s)
            {
                stream->error = CDI_FS_ERROR_INTERNAL;
                return 0;
            }
            strncpy(baudrate_s, res->res.name, trenner - res->res.name );
            baudrate_s[(trenner - res->res.name)] = 0;
            baudrate = atoi(baudrate_s);
            free(baudrate_s);

            char* buffer = malloc(2);
            if (!buffer)
            {
                stream->error = CDI_FS_ERROR_INTERNAL;
                return 0;
            }
            buffer[1] = 0;
            buffer[0] = trenner[1];
            bits = (uint8_t)atoi(buffer);
            buffer[0] = trenner[3];
            stopbits = (uint8_t)atoi(buffer);
            free(buffer);

            // Paritaet feststellen.
            if (strncmp(trenner+2, "n", 1) == 0)
            {
                parity = PARITY_NONE;
            }
            else if (strncmp(trenner+2, "o", 1) == 0)
            {
                parity = PARITY_ODD;
            }
            else if (strncmp(trenner+2, "e", 1) == 0)
            {
                parity = PARITY_EVEN;
            }
            else if (strncmp(trenner+2, "s", 1) == 0)
            {
                parity = PARITY_SPACE;
            }
            else if (strncmp(trenner+2, "m", 1) == 0)
            {
                parity = PARITY_MARK;
            }
            else
            {
                stream->error = CDI_FS_ERROR_ONS;
                return 0;
            }
        }
        else
        {
            baudrate = atoi(res->res.name);
        }

        printf("[SERIAL] Baudrate: %d, Bits: %d, Parity: %d, Stopbits: %d\n",
                baudrate, bits, parity, stopbits);

        if (baudrate <= 0 || baudrate > 115200)
        {
            stream->error = CDI_FS_ERROR_ONS;
            return 0;
        }

        if (bits < 5 || bits > 8)
        {
            stream->error = CDI_FS_ERROR_ONS;
            return 0;
        }

        if (stopbits > 1)
        {
            stream->error = CDI_FS_ERROR_ONS;
            return 0;
        }

        if (parent->baseport)
        {
            serial_init(parent->baseport,baudrate,bits,parity,stopbits);
        }
        return 1;
    }
    else
    {
        stream->error = CDI_FS_ERROR_ONS;
        return 0;
    }
}

int serial_fs_res_remove_class(struct cdi_fs_stream* stream,
            cdi_fs_res_class_t class)
{
    struct serial_fs_res* res = (struct serial_fs_res*) stream->res;
    struct serial_fs_res* parent = (struct serial_fs_res*) res->res.parent;


    if (class == CDI_FS_CLASS_FILE && res->baseport == 0)
    {
        res->res.file = NULL;
        serial_free(parent->baseport);
        return 1;
    }
    else
    {
        stream->error = CDI_FS_ERROR_ONS;
        return 0;
    }
}

int serial_fs_res_rename(struct cdi_fs_stream* stream, const char* name)
{
    stream->error = CDI_FS_ERROR_ONS;
    return 0;
}

int serial_fs_res_remove(struct cdi_fs_stream* stream)
{
    struct serial_fs_res* res = (struct serial_fs_res*) stream->res;
    struct serial_fs_res* parent = (struct serial_fs_res*) res->res.parent;


    if (res->baseport != 0) {
        stream->error = CDI_FS_ERROR_ONS;
        return 0;
    }

    // Link aus der Liste der Vater-Resource entfernen
    size_t i;
    struct serial_fs_res* child;
    for (i=0;(child = cdi_list_get(res->res.parent->children,i));i++) {
        if (child==res) {
            cdi_list_remove(res->res.parent->children,i);
            break;
        }
    }

    parent->in_use--;

    serial_fs_res_destroy(res);

    return 1;
}

int serial_fs_res_destroy(struct serial_fs_res* res)
{
    struct serial_fs_res *child;
    while ((child = cdi_list_pop(res->res.children))) serial_fs_res_destroy(child);
    cdi_list_destroy(res->res.children);
    serial_free(res->baseport);
    free(res);
    return 1;
}
