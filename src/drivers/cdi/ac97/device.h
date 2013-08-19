/******************************************************************************
 * Copyright (c) 2010 Max Reitz                                               *
 *                                                                            *
 * Permission is hereby granted,  free of charge,  to any person  obtaining a *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction,  including without limitation *
 * the rights to use, copy,  modify, merge, publish,  distribute, sublicense, *
 * and/or  sell copies of the  Software,  and to permit  persons to whom  the *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED,  INCLUDING BUT NOT LIMITED TO THE WARRANTIES  OF MERCHANTABILITY, *
 * FITNESS  FOR A PARTICULAR  PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY,  WHETHER IN AN ACTION OF CONTRACT,  TORT OR OTHERWISE,  ARISING *
 * FROM,  OUT OF  OR IN CONNECTION  WITH  THE SOFTWARE  OR THE  USE OR  OTHER *
 * DEALINGS IN THE SOFTWARE.                                                  *
 ******************************************************************************/

#ifndef DEVICE_H
#define DEVICE_H

#include <stddef.h>
#include <stdint.h>

#include "cdi.h"
#include "cdi/audio.h"
#include "cdi/mem.h"
#include "cdi/pci.h"

// Some I/O port offsets
// NAMBAR
#define NAM_RESET          0x0000
#define NAM_MASTER_VOLUME  0x0002
#define NAM_MONO_VOLUME    0x0006
#define NAM_PC_BEEP        0x000A
#define NAM_PCM_VOLUME     0x0018
#define NAM_EXT_AUDIO_ID   0x0028
#define NAM_EXT_AUDIO_STC  0x002A
#define NAM_FRONT_SPLRATE  0x002C
#define NAM_LR_SPLRATE     0x0032
// NABMBAR
#define NABM_POBDBAR       0x0010
#define NABM_POCIV         0x0014
#define NABM_POLVI         0x0015
#define NABM_PISTATUS      0x0006
#define NABM_POSTATUS      0x0016
#define NABM_MCSTATUS      0x0026
#define NABM_POPICB        0x0018
#define NABM_PICONTROL     0x000B
#define NABM_POCONTROL     0x001B
#define NABM_MCCONTROL     0x002B
#define NABM_GLB_STAT      0x0030

// Buffer flags which may be set
// Buffer underrun policy: If set, output 0 after the last sample, if there's no
// new buffer; else, output the last sample.
#define FLAG_BUP (1 << 14)
// Interrupt on completion: Request an interrupt when having completed the
// buffer
#define FLAG_IOC (1 << 15)

// A descriptor describing a buffer
struct buf_desc
{
    // Physical address
    uint32_t buffer;
    // Number of samples contained
    uint16_t length;
    // Flags (BUP and/or IOC)
    uint16_t flags;
} __attribute__((packed));

// A structure describing an AC97 card
struct ac97_device {
    // The two I/O spaces
    uint16_t nambar, nabmbar;

    // The buffer descriptor array
    struct buf_desc* bd;
    // The buffer's virtual addresses
    void** vbd;

    // Pointer to the output stream
    struct cdi_audio_stream* output;
};

struct ac97_stream {
    struct cdi_audio_stream stream;
    struct ac97_device* device;
};

struct ac97_card {
    struct cdi_audio_device card;
    struct ac97_device* device;
};

struct cdi_device* ac97_init_device(struct cdi_bus_data* bus_data);
void ac97_remove_device(struct cdi_device* device);

int ac97_transfer_data(struct cdi_audio_stream* stream, size_t buffer,
    struct cdi_mem_area* memory, size_t offset);
cdi_audio_status_t ac97_change_status(struct cdi_audio_device* device,
    cdi_audio_status_t status);
void ac97_set_volume(struct cdi_audio_stream* stream, uint8_t volume);
int ac97_set_sample_rate(struct cdi_audio_stream* stream, int sample_rate);
void ac97_get_position(struct cdi_audio_stream* stream,
    cdi_audio_position_t* position);
int ac97_set_channel_count(struct cdi_audio_device* dev, int channels);

#endif

