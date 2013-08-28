/*
 * Copyright (c) 2010 The tyndur Project. All rights reserved.
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cdi/pci.h"
#include "cdi/misc.h"
#include "cdi/printf.h"

#include "device.h"

#define DRIVER_NAME "hdaudio"

#define DPRINTF(fmt, ...) \
    do { CdiPrintf(DRIVER_NAME ": " fmt, ##__VA_ARGS__); } while(0)

static struct cdi_audio_driver driver;

static void hda_handle_interrupt(struct cdi_device* device)
{
    struct hda_device* hda = (struct hda_device*) device;
    uint32_t isr = reg_inl(hda, REG_INTSTS);
    uint8_t sts = reg_inb(hda, REG_O0_STS);

    DPRINTF("hda_handle_interrupt, status %x, O0 status %x [%d]\n",
        isr, sts, hda->buffers_completed);

    /* An interrupt for stream 1 means that a buffer has completed */
    if (sts & 0x4) {
        cdi_audio_buffer_completed(&hda->output.stream,
            hda->buffers_completed);

        hda->buffers_completed++;
        hda->buffers_completed %= BDL_SIZE;
    }

    /* Reset interrupt status registers */
    reg_outl(hda, REG_INTSTS, isr);
    reg_outb(hda, REG_O0_STS, sts);
}

static void setup_corb(struct hda_device* hda)
{
    uint8_t reg;
    uint64_t corb_base;

    reg = reg_inb(hda, REG_CORBSIZE);

    /* Check CORB size capabilities and choose the largest size */
    if (reg & (1 << 6)) {
        hda->corb_entries = 256;
        reg |= 0x2;
    } else if (reg & (1 << 5)) {
        hda->corb_entries = 16;
        reg |= 0x1;
    } else if (reg & (1 << 4)) {
        hda->corb_entries = 2;
        reg |= 0x0;
    } else {
        printf("hdaudio: No supported CORB size!\n");
    }

    reg_outb(hda, REG_CORBSIZE, reg);

    /* Set CORB base address */
    corb_base = (uintptr_t) hda->rings->paddr.items[0].start + 0;
    reg_outl(hda, REG_CORBLBASE, corb_base & 0xffffffff);
    reg_outl(hda, REG_CORBUBASE, corb_base >> 32);

    /* Start DMA engine */
    reg_outb(hda, REG_CORBCTL, 0x02);
}

static void setup_rirb(struct hda_device* hda)
{
    uint8_t reg;
    uint64_t rirb_base;

    reg = reg_inb(hda, REG_RIRBSIZE);

    /* Check RIRB size capabilities and choose the largest size */
    if (reg & (1 << 6)) {
        hda->rirb_entries = 256;
        reg |= 0x2;
    } else if (reg & (1 << 5)) {
        hda->rirb_entries = 16;
        reg |= 0x1;
    } else if (reg & (1 << 4)) {
        hda->rirb_entries = 2;
        reg |= 0x0;
    } else {
        printf("hdaudio: No supported RIRB size!\n");
    }

    reg_outb(hda, REG_RIRBSIZE, reg);

    /* Set RIRB base address */
    rirb_base = (uintptr_t) hda->rings->paddr.items[0].start + 1024;
    reg_outl(hda, REG_RIRBLBASE, rirb_base & 0xffffffff);
    reg_outl(hda, REG_RIRBUBASE, rirb_base >> 32);

    reg_outb(hda, REG_RINTCNT, 0x42); /* TODO Is this only needed for qemu? */

    /* Start DMA engine */
    reg_outb(hda, REG_RIRBCTL, 0x02);
}

static void corb_write(struct hda_device* hda, uint32_t verb)
{
    uint16_t wp = reg_inw(hda, REG_CORBWP) & 0xff;
    uint16_t rp;
    uint16_t next;

    /* Wait until there's a free entry in the CORB */
    next = (wp + 1) % hda->corb_entries;

    do {
        rp = reg_inw(hda, REG_CORBRP) & 0xff;
    } while (next == rp);

    /* Write to CORB */
    hda->corb[next] = verb;
    reg_outw(hda, REG_CORBWP, next);
}

static void rirb_read(struct hda_device* hda, uint64_t* response)
{
    uint16_t wp;
    uint16_t rp = hda->rirbrp;

    /* Wait for an unread entry in the RIRB */
    do {
       wp = reg_inw(hda, REG_RIRBWP) & 0xff;
    } while (wp == rp);

    /* Read from RIRB */
    rp = (rp + 1) % hda->rirb_entries;
    hda->rirbrp = rp;

    *response = hda->rirb[rp];
    reg_outb(hda, REG_RIRBSTS, 0x5); /* TODO Is this only needed for qemu? */
}

static uint32_t codec_query(struct hda_device* hda, int codec, int nid,
    uint32_t payload)
{
    uint64_t response;
    uint32_t verb =
        ((codec & 0xf) << 28) |
        ((nid & 0xff) << 20) |
        (payload & 0xfffff);

    corb_write(hda, verb);
    rirb_read(hda, &response);

    return response & 0xffffffff;
}

static void configure_output_widget(struct hda_device* hda)
{
    /* Set sample format */
    codec_query(hda, hda->output.codec, hda->output.nid,
        VERB_SET_FORMAT |
        BITS_16 | hda->output.sample_rate |
        (hda->output.num_channels - 1));
}

static void init_output_widget(struct hda_device* hda)
{
    /* CDI stream properties */
    hda->output.stream.device = &hda->audio;
    hda->output.stream.num_buffers = BDL_SIZE;
    hda->output.stream.buffer_size = BUFFER_SIZE / 2;
    hda->output.stream.sample_format = CDI_AUDIO_16SI;

    /* Select the right HDA stream/channel (0:0) */
    codec_query(hda, hda->output.codec, hda->output.nid,
        VERB_SET_STREAM_CHANNEL | 0x10);

    /* Set sample rate etc. */
    hda->output.sample_rate = SR_48_KHZ;
    hda->output.num_channels = 2;
    configure_output_widget(hda);
}

static int find_output_widget(struct hda_device* hda, int codec)
{
    uint32_t param;
    int num_fg, num_widgets;
    int fg_start, widgets_start;
    int i, j;

    param = codec_query(hda, codec, 0, VERB_GET_PARAMETER | PARAM_NODE_COUNT);

    num_fg = param & 0xff;
    fg_start = (param >> 16) & 0xff;

    DPRINTF("    %d function groups starting at ID %d\n", num_fg, fg_start);

    for (i = 0; i < num_fg; i++) {
        param = codec_query(hda, codec, fg_start + i,
            VERB_GET_PARAMETER | PARAM_NODE_COUNT);

        num_widgets = param & 0xff;
        widgets_start = (param >> 16) & 0xff;

        DPRINTF("    %d widgets starting at ID %d\n",
            num_widgets, widgets_start);
        for (j = 0; i < num_widgets; j++) {
            param = codec_query(hda, codec, widgets_start + j,
                VERB_GET_PARAMETER | PARAM_AUDIO_WID_CAP);
            if ((param != 0) && ((param & 0xf00000) == 0)) {
                DPRINTF("    Audio output at ID %d!\n", widgets_start + j);

                hda->output.codec = codec;
                hda->output.nid = widgets_start + j;

                return 0;
            }
        }
    }

    return -1;
}

static void hda_enumerate_codecs(struct hda_device* hda)
{
    uint16_t statests;
    int i;

    DPRINTF("hda_enumerate_codecs\n");
    statests = reg_inw(hda, REG_STATESTS);

    /*
     * Our model of the hardware is completely wrong: Our hda_device is the PCI
     * card when in fact it should be a device on an HDA bus which is provided
     * by the PCI device. This mean that we're limited to using a single
     * output.
     *
     * So let's just pick the first output widget and use that one.
     */
    for (i = 0; i < 15; i++) {
        if ((statests & (1 << i))) {
            DPRINTF("   Found codec at %d\n", i);
            if (find_output_widget(hda, i)) {
                return;
            }
        }
    }
}

static void hda_reset(struct hda_device* hda)
{
    DPRINTF("hda_reset\n");

    /* Must clear CORB/RIRB RUN bits before reset */
    reg_outl(hda, REG_CORBCTL, 0);
    reg_outl(hda, REG_RIRBCTL, 0);
    while ((reg_inl(hda, REG_CORBCTL) & CORBCTL_CORBRUN)
        || (reg_inl(hda, REG_RIRBCTL) & RIRBCTL_RIRBRUN));

    /* Reset the CRST bit and wait until hardware is in reset */
    reg_outl(hda, REG_GCTL, 0);
    while ((reg_inl(hda, REG_GCTL) & GCTL_RESET));

    /* TODO Need a delay here? */

    /* Take the hardware out of reset */
    reg_outl(hda, REG_GCTL, GCTL_RESET);
    while ((reg_inl(hda, REG_GCTL) & GCTL_RESET) == 0);

    /* We want to get all interrupts */
    reg_outw(hda, REG_WAKEEN, 0xffff);
    reg_outl(hda, REG_INTCTL, 0x800000ff);

    /* Setup data structures */
    setup_corb(hda);
    setup_rirb(hda);
    DPRINTF("CORB size: %d entries; RIRB size: %d entries\n",
        hda->corb_entries, hda->rirb_entries);

    /* Need to wait 25 frames (521 µs) before enumerating codecs */
    cdi_sleep_ms(1);
    hda_enumerate_codecs(hda);
}

static void init_stream_descriptor(struct hda_device* hda)
{
    uint64_t bdl_base, dma_pos_base;
    int i;

    /* Set some Stream Descriptor registers for Output 0 */
    reg_outb(hda, REG_O0_CTLU, 0x10);
    reg_outl(hda, REG_O0_CBL, BDL_SIZE * BUFFER_SIZE);
    reg_outw(hda, REG_O0_STLVI, BDL_SIZE - 1);

    /* Setup Buffer Description List */
    bdl_base = (uintptr_t) hda->rings->paddr.items[0].start + 3072;
    reg_outl(hda, REG_O0_BDLPL, bdl_base & 0xffffffff);
    reg_outl(hda, REG_O0_BDLPU, bdl_base >> 32);

    for (i = 0; i < BDL_SIZE; i++) {
        hda->bdl[i].paddr =
            hda->buffer->paddr.items[0].start + (i * BUFFER_SIZE);
        hda->bdl[i].length = BUFFER_SIZE;
        hda->bdl[i].flags = 1;
    }

    memset(hda->buffer->vaddr, 0, BDL_SIZE * BUFFER_SIZE);

    /* Initialise DMA Position in Buffer structure */
    for (i = 0; i < 8; i++) {
        hda->dma_pos[i] = 0;
    }

    dma_pos_base = (uintptr_t) hda->rings->paddr.items[0].start + 3072
        + BDL_BYTES_ROUNDED;
    reg_outl(hda, REG_DPLBASE, (dma_pos_base & 0xffffffff) | 0x1); /* TODO */
    reg_outl(hda, REG_DPUBASE, dma_pos_base >> 32);
}

static struct cdi_device* hda_init_device(struct cdi_bus_data* bus_data)
{
    struct cdi_pci_device* pci = (struct cdi_pci_device*) bus_data;
    struct hda_device* hda;
    int i;

    /* Check PCI class/subclass */
    if (pci->class_id != PCI_CLASS_MULTIMEDIA ||
        pci->subclass_id != PCI_SUBCLASS_MM_HDAUDIO)
    {
        return NULL;
    }

    /* Initialise device structure */
//    hda = calloc(1, sizeof(*hda));
    hda = malloc(sizeof(*hda));
    memset(hda, 0, sizeof(*hda));

    hda->rings = cdi_mem_alloc(1024 + 2048 + BDL_BYTES_ROUNDED + 128,
        CDI_MEM_PHYS_CONTIGUOUS | 7);
    if (hda->rings == NULL) {
        goto fail;
    }

    hda->corb       = (uint32_t*) ((uintptr_t) hda->rings->vaddr + 0);
    hda->rirb       = (uint64_t*) ((uintptr_t) hda->rings->vaddr + 1024);
    hda->bdl        = (void*    ) ((uintptr_t) hda->rings->vaddr + 3072);
    hda->dma_pos    = (uint64_t*) ((uintptr_t) hda->rings->vaddr + 3072
                        + BDL_BYTES_ROUNDED);

    hda->buffer = cdi_mem_alloc(BDL_SIZE * BUFFER_SIZE,
        CDI_MEM_PHYS_CONTIGUOUS | 7);
    if (hda->buffer == NULL) {
        goto fail;
    }

    /* Get information from PCI config space */
    cdi_list_t reslist = pci->resources;
    struct cdi_pci_resource* res;
    for (i = 0; (res = cdi_list_get(reslist, i)); i++) {
        if (res->type == CDI_PCI_MEMORY) {
            hda->mmio = cdi_mem_map(res->start, res->length);
            if (hda->mmio == NULL) {
                goto fail;
            }
        }
    }

    cdi_register_irq(pci->irq, hda_handle_interrupt, &hda->audio.dev);
    cdi_pci_alloc_ioports(pci);

    /* Initialise hardware */
    hda_reset(hda);
    init_output_widget(hda);
    init_stream_descriptor(hda);

    hda->audio.record = 0;
    hda->audio.streams = cdi_list_create();
    cdi_list_push(hda->audio.streams, &hda->output.stream);

    driver.set_volume(&hda->output.stream, 255);

    return &hda->audio.dev;

fail:
    if (hda->buffer) {
        cdi_mem_free(hda->buffer);
    }
    if (hda->rings) {
        cdi_mem_free(hda->rings);
    }

    free(hda);
    return NULL;
}

static void hda_remove_device(struct cdi_device* device)
{
    /* TODO */
}


static int hda_driver_init(void)
{
    cdi_driver_init(&driver.drv);

    return 0;
}

static int hda_driver_destroy(void)
{
    cdi_driver_destroy(&driver.drv);

    return 0;
}

static void hda_set_volume(struct cdi_audio_stream* stream, uint8_t volume)
{
    struct hda_device* hda = (struct hda_device*) stream->device;
    int meta = 0xb000; /* Output Amp, Left and Right */

    if (volume == 0) {
        /* Set the mute bit */
        volume = 0x80;
    } else {
        /* Scale to 7-bit value */
        volume >>= 1;
    }

    codec_query(hda, hda->output.codec, hda->output.nid,
        VERB_SET_AMP_GAIN_MUTE | meta | volume);
}

static int hda_set_sample_rate(struct cdi_audio_stream* stream,
    int sample_rate)
{
    struct hda_device* hda = (struct hda_device*) stream->device;

    switch (sample_rate) {
        case 44100:
            hda->output.sample_rate = SR_44_KHZ;
            break;
        default:
            sample_rate = 48000;
            hda->output.sample_rate = SR_48_KHZ;
            break;
    }

    configure_output_widget(hda);

    return sample_rate;
}

static int hda_set_channel_count(struct cdi_audio_device* dev, int channels)
{
    struct hda_device* hda = (struct hda_device*) dev;

    if (channels < 1 || channels > 2) {
        channels = 2;
    }

    hda->output.num_channels = channels;
    configure_output_widget(hda);

    return channels;
}

static int hda_transfer_data(struct cdi_audio_stream* stream, size_t buffer,
    struct cdi_mem_area* memory, size_t offset)
{
    struct hda_device* hda = (struct hda_device*) stream->device;

    DPRINTF("hda_transfer_data: buf %d, off %d\n", buffer, offset);

    if (buffer >= BDL_SIZE || offset >= SAMPLES_PER_BUFFER) {
        return -1;
    }

    memcpy((uint8_t*)hda->buffer->vaddr + buffer * BUFFER_SIZE + (offset * 2),
        memory->vaddr,
        (SAMPLES_PER_BUFFER - offset) * 2);

    return 0;
}

static cdi_audio_status_t hda_change_status(struct cdi_audio_device* device,
    cdi_audio_status_t status)
{
    struct hda_device* hda = (struct hda_device*) device;
    uint16_t ctl;

    DPRINTF("hda_change_status: %d\n", status);

    if (status == CDI_AUDIO_PLAY) {
        ctl = SDCTL_RUN | SDCTL_IOCE;
    } else {
        ctl = 0;
    }

    reg_outw(hda, REG_O0_CTLL, ctl);

    return status;
}

static void hda_get_position(struct cdi_audio_stream* stream,
    cdi_audio_position_t* position)
{
    struct hda_device* hda = (struct hda_device*) stream->device;
    uint32_t pos = hda->dma_pos[4] & 0xffffffff;

    position->buffer = pos / BUFFER_SIZE;
    position->frame = (pos % BUFFER_SIZE) / 2;

    DPRINTF("hda_get_position: %d/%d\n", position->buffer, position->frame);
}

static struct cdi_audio_driver driver = {
    .drv = {
        .name           = DRIVER_NAME,
        .type           = CDI_AUDIO,
        .bus            = CDI_PCI,
        .init           = hda_driver_init,
        .destroy        = hda_driver_destroy,
        .init_device    = hda_init_device,
        .remove_device  = hda_remove_device,
    },

    .set_volume             = hda_set_volume,
    .set_sample_rate        = hda_set_sample_rate,
    .set_number_of_channels = hda_set_channel_count,
    .transfer_data          = hda_transfer_data,
    .change_device_status   = hda_change_status,
    .get_position           = hda_get_position,
};

CDI_DRIVER(DRIVER_NAME, driver)
