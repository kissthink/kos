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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cdi.h"
#include "cdi/audio.h"
#include "cdi/io.h"
#include "cdi/lists.h"
#include "cdi/mem.h"
#include "cdi/misc.h"
#include "cdi/pci.h"

#include "device.h"

#define SAMPLES_PER_BUFFER 0xFFFE

static void ac97_isr(struct cdi_device* device);

struct cdi_device* ac97_init_device(struct cdi_bus_data* bus_data)
{
    struct cdi_pci_device* pci = (struct cdi_pci_device*) bus_data;

    // Search for a usable device
    if ((pci->vendor_id != 0x8086) || ((pci->device_id != 0x2415) &&
        (pci->device_id != 0x2425) &&  (pci->device_id != 0x2445)))
    {
        return NULL;
    }

    struct ac97_device* dev = calloc(1, sizeof(*dev));

    cdi_pci_alloc_ioports(pci);

    // Get the I/O resources (NAMBAR and NABMBAR)
    struct cdi_pci_resource* res;
    int i;
    for (i = 0; (res = cdi_list_get(pci->resources, i)); i++) {
        if (res->type == CDI_PCI_IOPORTS) {
            if (!res->index) {
                dev->nambar = res->start;
            } else if (res->index == 1) {
                dev->nabmbar = res->start;
            }
        }
    }

    if (!dev->nambar || !dev->nabmbar) {
        free(dev);
        cdi_pci_free_ioports(pci);
        return NULL;
    }

    #ifdef CDI_PCI_DIRECT_ACCESS
    // If possible, activate the I/O resources and the busmaster capability
    cdi_pci_config_writew(pci, 4, cdi_pci_config_readw(pci, 4) | 5);
    #endif

    struct ac97_card* ac97_card = malloc(sizeof(*ac97_card));
    struct ac97_stream* ac97_stream = malloc(sizeof(*ac97_stream));

    // Define the CDI.audio sound card
    ac97_card->card.record = 0;
    ac97_card->card.streams = cdi_list_create();
    ac97_card->device = dev;

    // And the only stream
    ac97_stream->stream.device = &ac97_card->card;
    ac97_stream->stream.num_buffers = 32;
    ac97_stream->stream.buffer_size = SAMPLES_PER_BUFFER;
    ac97_stream->stream.sample_format = CDI_AUDIO_16SI;

    ac97_stream->device = dev;

    dev->output = &ac97_stream->stream;

    cdi_list_push(ac97_card->card.streams, ac97_stream);

    // Register the IRQ
    cdi_register_irq(pci->irq, &ac97_isr, &ac97_card->card.dev);

    // Reset the card
    cdi_outw(dev->nambar + NAM_RESET, 42);

    cdi_sleep_ms(100);

    // And still something...
    cdi_outw(dev->nabmbar + NABM_PICONTROL, 0x02);
    cdi_outw(dev->nabmbar + NABM_POCONTROL, 0x02);
    cdi_outw(dev->nabmbar + NABM_MCCONTROL, 0x02);

    cdi_sleep_ms(100);

    // Set volume to 100 %
    cdi_outw(dev->nambar + NAM_MASTER_VOLUME, 0x0000);
    cdi_outw(dev->nambar + NAM_MONO_VOLUME, 0x00);
    cdi_outw(dev->nambar + NAM_PC_BEEP, 0x00);
    cdi_outw(dev->nambar + NAM_PCM_VOLUME, 0x0000);

    cdi_sleep_ms(10);

    // Test if sample rate is adjustable
    if (!(cdi_inw(dev->nambar + NAM_EXT_AUDIO_ID) & 1)) {
        // No it isn't; it's fixed to 48 kHz
        ac97_stream->stream.fixed_sample_rate = 48000;
    } else {
        // Yes, it is
        ac97_stream->stream.fixed_sample_rate = 0;

        // Use that capability (Variable Audio Rate)
        cdi_outw(dev->nambar + NAM_EXT_AUDIO_STC,
            cdi_inw(dev->nambar + NAM_EXT_AUDIO_STC) | 1);

        cdi_sleep_ms(10);

        // Set the sample rate to 44.1 kHz
        cdi_outw(dev->nambar + NAM_FRONT_SPLRATE, 44100);
        cdi_outw(dev->nambar + NAM_LR_SPLRATE, 44100);
    }

    // TODO: Error handling

    // Allocate the buffer descriptor array
    struct cdi_mem_area* bd = cdi_mem_alloc(32 * sizeof(dev->bd[0]),
        3 | CDI_MEM_DMA_4G | CDI_MEM_PHYS_CONTIGUOUS);

    dev->bd  = bd->vaddr;
    dev->vbd = calloc(32, sizeof(dev->vbd[0]));

    for (i = 0; i < 32; i++)
    {
        // Allocate each buffer
        struct cdi_mem_area* buffer = cdi_mem_alloc(SAMPLES_PER_BUFFER * 4,
            12 | CDI_MEM_DMA_4G | CDI_MEM_PHYS_CONTIGUOUS);

        dev->bd[i].buffer = buffer->paddr.items->start;
        dev->bd[i].length = SAMPLES_PER_BUFFER;
        // Generate an IRQ on completion
        dev->bd[i].flags = FLAG_IOC;
        dev->vbd[i] = buffer->vaddr;

        // Zero the buffer
        memset(buffer->vaddr, 0, SAMPLES_PER_BUFFER * 4);
    }

    // Set the base address of the descriptor array
    cdi_outl(dev->nabmbar + NABM_POBDBAR, bd->paddr.items->start);

    // Return the new device
    return &ac97_card->card.dev;
}

void ac97_remove_device(struct cdi_device* device)
{
    // TODO: Well...
    (void) device;
}

int ac97_transfer_data(struct cdi_audio_stream* stream, size_t buffer,
    struct cdi_mem_area* memory, size_t offset)
{
    // Check if buffer index is valid
    if (buffer >= 32) {
        return -1;
    }

    if (offset >= SAMPLES_PER_BUFFER) {
        return -1;
    }

    struct ac97_stream* s = (struct ac97_stream*) stream;

    // Copy the content to the AC97 buffer
    memcpy((uint16_t*) s->device->vbd[buffer] + offset, memory->vaddr,
        (SAMPLES_PER_BUFFER - offset) * 2);

    return 0;
}

cdi_audio_status_t ac97_change_status(struct cdi_audio_device* device,
    cdi_audio_status_t status)
{
    struct ac97_device* dev = ((struct ac97_card*) device)->device;

    if (status == CDI_AUDIO_PLAY) {
        // Start playback
        cdi_outb(dev->nabmbar + NABM_POCONTROL, 0x11);
        // Set last valid buffer index to 31
        cdi_outb(dev->nabmbar + NABM_POLVI, 31);
    } else if (status == CDI_AUDIO_STOP) {
        // Stop playback
        cdi_outb(dev->nabmbar + NABM_POCONTROL, 0x00);
    }

    // Returns whether the card is active or not
    return cdi_inb(dev->nabmbar + NABM_POCONTROL) & 0x1;
}

void ac97_set_volume(struct cdi_audio_stream* stream, uint8_t volume)
{
    struct ac97_device* dev = ((struct ac97_stream*) stream)->device;

    // Calculate the AC97 volume (which is 63 - (volume * 63 / 255))
    int rvolume = volume * 63;
    rvolume /= 255;

    rvolume = 63 - rvolume;

    // And set it
    cdi_outw(dev->nambar + NAM_MASTER_VOLUME, (rvolume << 8) | rvolume);
}

int ac97_set_sample_rate(struct cdi_audio_stream* stream, int sample_rate)
{
    struct ac97_device* dev = ((struct ac97_stream*) stream)->device;

    if (stream->fixed_sample_rate) {
        // If the sample rate cannot be adjustet, return the fixed value
        return stream->fixed_sample_rate;
    }

    if (sample_rate > 0) {
        // Try to set the requested sample rate
        cdi_outw(dev->nambar + NAM_FRONT_SPLRATE, sample_rate);
        cdi_outw(dev->nambar + NAM_LR_SPLRATE, sample_rate);

        cdi_sleep_ms(10);
    }

    // And return the actual rate set
    return cdi_inw(dev->nambar + NAM_FRONT_SPLRATE);
}

void ac97_get_position(struct cdi_audio_stream* stream,
    cdi_audio_position_t* position)
{
    struct ac97_device* dev = ((struct ac97_stream*) stream)->device;

    position->buffer = cdi_inb(dev->nabmbar + NABM_POCIV);
    // POPICB contains the number of samples not yet read from the buffer,
    // we are required to return the number of frames played
    position->frame =
        (SAMPLES_PER_BUFFER - cdi_inw(dev->nabmbar + NABM_POPICB)) / 2;
}

int ac97_set_channel_count(struct cdi_audio_device* dev, int channels)
{
    (void) dev;
    (void) channels;

    // TODO: Is that really fixed or is there any way to use e.g. mono?
    return 2;
}

static void ac97_isr(struct cdi_device* device)
{
    struct ac97_device* dev = ((struct ac97_card*) device)->device;
    static int last_buf = 0;

    int pi = cdi_inb(dev->nabmbar + NABM_PISTATUS) & 0x1C;
    int po = cdi_inb(dev->nabmbar + NABM_POSTATUS) & 0x1C;
    int mc = cdi_inb(dev->nabmbar + NABM_MCSTATUS) & 0x1C;

    // Check if just anything happened
    if (!pi && !po && !mc) {
        return;
    }

    // Tell the card we handled everything (no, we don't, but who cares).
    cdi_outb(dev->nabmbar + NABM_PISTATUS, pi);
    cdi_outb(dev->nabmbar + NABM_POSTATUS, po);
    cdi_outb(dev->nabmbar + NABM_MCSTATUS, mc);

    if (po & (1 << 3)) {
        int current = cdi_inb(dev->nabmbar + NABM_POCIV);

        if (current == last_buf) {
            // This happens sometimes, though I don't know why (for the last
            // buffer, i.e. buffer 30). This fix is at least enough for qemu.
            current = (current + 1) & 0x1F;
            cdi_outb(dev->nabmbar + NABM_POCONTROL, 0x11);
        }

        last_buf = current;

        // Tell the CDI library about the finished buffer
        cdi_audio_buffer_completed(dev->output, (current - 1) & 0x1F);
    }
}

