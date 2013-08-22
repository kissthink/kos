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

#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <stdint.h>

// Basisadressen.
uint16_t COM1_BASE;
uint16_t COM2_BASE;
uint16_t COM3_BASE;
uint16_t COM4_BASE;

// Register-Port-Offsets
#define REG_TR  0x00 // Bei nicht gesetztem DLAB
#define REG_IER 0x01 // Bei nicht gesetztem DLAB
#define REG_IIR 0x02
#define REG_FCR 0x02
#define REG_LCR 0x03
#define REG_MCR 0x04
#define REG_LSR 0x05
#define REG_MSR 0x06
#define REG_SCR 0x07

// Parity-Bits
typedef enum {
    PARITY_NONE  = 0,
    PARITY_ODD   = 4,
    PARITY_EVEN  = 6,
    PARITY_MARK  = 5,
    PARITY_SPACE = 7
} parity_t;

// Funktionen
int serial_init(uint16_t base, uint32_t baudrate, uint8_t bits, uint8_t parity,
                uint8_t stopbits);
void serial_send(uint16_t base, uint8_t data);
int serial_can_send(uint16_t base);
uint8_t serial_recv(uint16_t base);
int serial_can_recv(uint16_t base);
void serial_free(uint16_t base);

#endif /* _SERIAL_H_ */
