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
#include "stdio.h"
#include "cdi/misc.h"

size_t serial_fs_file_read(struct cdi_fs_stream* stream, uint64_t start,
    size_t size, void* data)
{
    struct serial_fs_res* res = (struct serial_fs_res*) stream->res;
    struct serial_fs_res* parent = (struct serial_fs_res*) res->res.parent;
    size_t ptr = 0;
    int retry = 0;

    if (parent->baseport != 0)
    {
        while (ptr < size)
        {
            if (serial_can_recv(parent->baseport))
            {
                ((uint8_t*)data)[ptr++] = serial_recv(parent->baseport);
                retry = 0;
            }
            else
            {
                // Timeout-Ersatz, bei zu schnellen Rechnern benoetigt um
                // brauchbar zu lesen.
                retry++;
                if (retry > 3)
                {
                    stream->error = CDI_FS_ERROR_EOF;
                    return ptr;
                }
                else
                {
                    cdi_sleep_ms(1);
                }
            }
        }
    }
    else
    {
        stream->error = CDI_FS_ERROR_ONS;
        return 0;
    }
    return size;
}

size_t serial_fs_file_write(struct cdi_fs_stream* stream, uint64_t start,
    size_t size, const void* data)
{
    struct serial_fs_res* res = (struct serial_fs_res*) stream->res;
    struct serial_fs_res* parent = (struct serial_fs_res*) res->res.parent;

    if (parent->baseport != 0)
    {
        size_t ptr = 0;
        while (ptr < size)
        {
            if (serial_can_send(parent->baseport))
            {
                serial_send(parent->baseport, *(uint8_t*)(data + ptr++));
            }
        }
    }
    else
    {
        stream->error = CDI_FS_ERROR_ONS;
        return 0;
    }
    return size;
}

int serial_fs_file_truncate(struct cdi_fs_stream* stream, uint64_t size)
{
    return 0;
}
