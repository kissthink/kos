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

#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>

#include "cdi/audio.h"
#include "cdi/mem.h"

#define BDL_SIZE                      4
#define BUFFER_SIZE             0x10000

#define BDL_BYTES_ROUNDED \
    ((BDL_SIZE * sizeof(struct hda_bdl_entry) + 127) & ~127)

#define SAMPLES_PER_BUFFER (BUFFER_SIZE / 2)

/** HD Audio Memory Mapped Registers */
enum hda_reg {
    REG_GCTL        = 0x08, ///< GCTL - Global Control
    REG_WAKEEN      = 0x0c, ///< WAKEEN - Wake Enable
    REG_STATESTS    = 0x0e, ///< STATESTS - State Change Status
    REG_INTCTL      = 0x20, ///< INTCTL - Interrupt Control
    REG_INTSTS      = 0x24, ///< INTSTS - Interrupt Status

    REG_CORBLBASE   = 0x40, ///< CORBLBASE - CORB Lower Base Address
    REG_CORBUBASE   = 0x44, ///< CORBUBASE - CORB Upper Base Address
    REG_CORBWP      = 0x48, ///< CORBWP - CORB Write Pointer
    REG_CORBRP      = 0x4a, ///< CORBRP - CORB Read Pointer
    REG_CORBCTL     = 0x4c, ///< CORBCTL - CORB Control
    REG_CORBSIZE    = 0x4e, ///< CORBSIZE - CORB size

    REG_RIRBLBASE   = 0x50, ///< RIRBLBASE - RIRB Lower Base Address
    REG_RIRBUBASE   = 0x54, ///< RIRBUBASE - RIRB Upper Base Address
    REG_RIRBWP      = 0x58, ///< RIRBWP - RIRB Write Pointer
    REG_RINTCNT     = 0x5a, ///< RINTCNT - Respone Interrupt Count
    REG_RIRBCTL     = 0x5c, ///< RIRBCTL - RIRB Control
    REG_RIRBSTS     = 0x5d, ///< RIRBSTS - RIRB Interrupt Status
    REG_RIRBSIZE    = 0x5e, ///< RIRBSIZE - RIRB size

    REG_DPLBASE     = 0x70, ///< DPLBASE - DMA Position Lower Base Address
    REG_DPUBASE     = 0x74, ///< DPUBASE - DMA Posiition Upper Base Address

    REG_O0_CTLL     = 0x100,    ///< Output 0 - Control Lower
    REG_O0_CTLU     = 0x102,    ///< Output 0 - Control Upper
    REG_O0_STS      = 0x103,    ///< Output 0 - Status
    REG_O0_CBL      = 0x108,    ///< Output 0 - Cyclic Buffer Length
    REG_O0_STLVI    = 0x10c,    ///< Output 0 - Last Valid Index
    REG_O0_BDLPL    = 0x118,    ///< Output 0 - BDL Pointer Lower
    REG_O0_BDLPU    = 0x11c,    ///< Output 0 - BDL Pointer Upper
};

/* GCTL bits */
enum hda_reg_gctl {
    GCTL_RESET  =   (1 << 0),
};

/* CORBCTL bits */
enum hda_reg_corbctl {
    CORBCTL_CORBRUN = (1 << 1),
};

/* RIRBCTL bits */
enum hda_reg_rirbctl {
    RIRBCTL_RIRBRUN = (1 << 1),
};

/* Stream Descriptor Control Register bits */
enum hda_reg_sdctl {
    SDCTL_RUN   =   0x2, ///< Enable DMA Engine
    SDCTL_IOCE  =   0x4, ///< Enable Interrupt on Complete
};

/** Represents an address of an output widget */
struct hda_output {
    struct cdi_audio_stream stream;

    uint8_t     codec;
    uint16_t    nid;

    uint32_t    sample_rate;
    int         num_channels;
};

/** Buffer Descriptor List Entry */
struct hda_bdl_entry {
    uint64_t paddr;
    uint32_t length;
    uint32_t flags;
} __attribute__((packed));

/** Represents the state of an HD Audio PCI card */
struct hda_device {
    struct cdi_audio_device audio;

    struct hda_output output;

    struct cdi_mem_area* mmio;
    uintptr_t mmio_base;


    struct cdi_mem_area*    rings;          ///< Buffer for CORB and RIRB
    uint32_t*               corb;           ///< Command Outbound Ring Buffer
    volatile uint64_t*      rirb;           ///< Response Inbound Ring Buffer
    struct hda_bdl_entry*   bdl;            ///< Buffer Descriptor List
    volatile uint64_t*      dma_pos;        ///< DMA Position in Current Buffer

    size_t                  corb_entries;   ///< Number of CORB entries
    size_t                  rirb_entries;   ///< Number of RIRB entries
    uint16_t                rirbrp;         ///< RIRB Read Pointer

    struct cdi_mem_area*    buffer;
    int                     buffers_completed;
};

static inline void reg_outl(struct hda_device* hda, int reg, uint32_t value) {
    volatile uint32_t* mmio = (uint32_t*) ((uint8_t*) hda->mmio->vaddr + reg);
    *mmio = value;
}

static inline uint32_t reg_inl(struct hda_device* hda, int reg) {
    volatile uint32_t* mmio = (uint32_t*) ((uint8_t*) hda->mmio->vaddr + reg);
    return *mmio;
}


static inline void reg_outw(struct hda_device* hda, int reg, uint16_t value) {
    volatile uint16_t* mmio = (uint16_t*) ((uint8_t*) hda->mmio->vaddr + reg);
    *mmio = value;
}

static inline uint16_t reg_inw(struct hda_device* hda, int reg) {
    volatile uint16_t* mmio = (uint16_t*) ((uint8_t*) hda->mmio->vaddr + reg);
    return *mmio;
}


static inline void reg_outb(struct hda_device* hda, int reg, uint8_t value) {
    volatile uint8_t* mmio = (uint8_t*) ((uint8_t*) hda->mmio->vaddr + reg);
    *mmio = value;
}

static inline uint8_t reg_inb(struct hda_device* hda, int reg) {
    volatile uint8_t* mmio = (uint8_t*) ((uint8_t*) hda->mmio->vaddr + reg);
    return *mmio;
}

enum codec_verbs {
    VERB_GET_PARAMETER      = 0xf0000,
    VERB_SET_STREAM_CHANNEL = 0x70600,
    VERB_SET_FORMAT         = 0x20000,
    VERB_SET_AMP_GAIN_MUTE  = 0x30000,
};

enum codec_parameters {
    PARAM_NODE_COUNT        = 0x04,
    PARAM_AUDIO_WID_CAP     = 0x09,
};

enum sample_format {
    SR_48_KHZ               = 0,
    SR_44_KHZ               = (1 << 14),
    BITS_32                 = (4 <<  4),
    BITS_16                 = (1 <<  4),
};


#endif
