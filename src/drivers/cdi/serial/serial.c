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

#include "serial.h"
#include "cdi.h"
#include "cdi/io.h"
#include "cdi/misc.h"

// Basisadressen.
uint16_t COM1_BASE = 0x3F8;
uint16_t COM2_BASE = 0x2F8;
uint16_t COM3_BASE = 0x3E8;
uint16_t COM4_BASE = 0x2E8;

/**
 *  Oeffnet einen Port an der angegeben Basisadresse unter angegeben Optionen.
 * @param base Basisport (Wird nicht geprueft!)
 * @param baudrate Baudrate halt. 115200 Maximal.
 * @param parity Paritaet.
 * @param bits Anzahl der Datenbits einer Uebertragung. 5-8
 * @param stopbits Anzahl der Stopbits
 * @return 0 bei Erfolg, sonst -1
 */
int serial_init(uint16_t base, uint32_t baudrate, uint8_t bits, uint8_t parity,
                uint8_t stopbits)
{
    uint16_t divisor = 0;
    int ports_allocated = -1;

    // Baudrate pruefen
    if (baudrate > 115200)
        return -1;

    // Ports holen
    ports_allocated = cdi_ioports_alloc(base, 8);
    if (ports_allocated == -1)
        return -1;

    // Interrupts deaktivieren
    cdi_outb(base+REG_IER,0x00);

    // DLAB setzen
    cdi_outb(base+REG_LCR,0x80);

    // Baudrate setzen
    divisor = 115200 / baudrate;
    cdi_outw(base, divisor);

    // Anzahl Bits, Paritaet, usw setzen (DLAB zurücksetzen)
    cdi_outb(base+REG_LCR,((parity&0x7)<<3)|((bits-5)&0x3)|((stopbits&0x1)<<2));

    // Initialisierung abschließen
    cdi_outb(base+REG_FCR,0xC7);
    cdi_outb(base+REG_MCR,0x0B);

    return 0;
}

/**
 * Sendet ein uint8 an den Port
 * @param base Der IO-Port der Schnittstelle an die etwas geschickt werden soll.
 * @param data uint8 an Daten
 */
void serial_send(uint16_t base, uint8_t data)
{
    cdi_outb(base,data);
}

/**
 * Prueft ob gesendet werden kann (1 Byte!)
 * @param base Der IO-Port der Schnittstelle, auf der gesendet werden soll.
 */
int serial_can_send(uint16_t base)
{
    if ((cdi_inb(base+REG_LSR)&0x20)==0)
        return 0;
    else
        return 1;
}

/**
 * Liest ein uint8 von dem Port
 * @param base Der IO-Port der Schnittstelle von der gelesen werden soll.
 */
uint8_t serial_recv(uint16_t base)
{
    return cdi_inb(base);
}

/**
 * Prueft, ob ein uint8 von dem Port gelesen werden kann.
 * @param base Der IO-Port der Schnittstelle von der gelesen werden soll.
 */
int serial_can_recv(uint16_t base)
{
    return (cdi_inb(base+REG_LSR)&1);
}

/**
 * Gibt den Port wieder frei.
 * @param base Der IO-Port der Schnittstelle die Freigegeben werden soll.
 */
void serial_free(uint16_t base)
{
    cdi_ioports_free(base, 8);
}
