/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _ATA_Controller_h_
#define _ATA_Controller_h_ 1

#include "mach/Device.h"
#include "mach/Disk.h"
#include "mach/Controller.h"
#include "mach/IOBase.h"
#include "mach/IOPort.h"
#include "util/RequestQueue.h"
#include "mach/IrqHandler.h"
#include "kern/Debug.h"

#define ATA_CMD_READ  0
#define ATA_CMD_WRITE 1

/** Base class for an ATA controller. */
class AtaController : public Controller, public RequestQueue, public IrqHandler
{
public:
    AtaController(Controller *pDev, int nController = 0) :
        Controller(pDev), m_nController(nController)
    {
        setSpecificType(String("ata-controller"));

        // Ensure we have no stupid children lying around.
        m_Children.clear();

        // Initialise the RequestQueue
        initialise();
    }
    virtual ~AtaController()
    {
    }

    virtual void getName(String &str) = 0;

    virtual uint64_t executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
                                  uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8) = 0;

    // IRQ handler callback.
    virtual bool irq(uint8_t number)
    {
        DBG::outln(DBG::Drivers, "AtaController: irq", number, " ignored");
        return false;
    }

    IOBase *m_pCommandRegs;
    IOBase *m_pControlRegs;
private:
    AtaController(const AtaController&) = delete;
    AtaController& operator=(const AtaController&) = delete;

protected:
    int m_nController;
};

#endif /* _ATA_Controller_h_ */
