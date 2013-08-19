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

#if 0

#include "AtaController.h"
#include <Log.h>
#include <machine/Machine.h>
#include <processor/Processor.h>

AtaController::AtaController(Controller *pDev, int nController) :
  Controller(pDev), m_pCommandRegs(0), m_pControlRegs(0), m_nController(nController)
{
  setSpecificType(String("ata-controller"));

  // Ensure we have no stupid children lying around.
  m_Children.clear();

  // Initialise our ports.
  for (unsigned int i = 0; i < m_Addresses.count(); i++)
  {
    /// \todo String operator== problem here.
    if (!strcmp(m_Addresses[i]->m_Name, "command") || !strcmp(m_Addresses[i]->m_Name, "bar0"))
      m_pCommandRegs = m_Addresses[i]->m_Io;
    if (!strcmp(m_Addresses[i]->m_Name, "control") || !strcmp(m_Addresses[i]->m_Name, "bar1"))
      m_pControlRegs = m_Addresses[i]->m_Io;
  }

  // Look for a floating bus
  if(m_pCommandRegs->read8(7) == 0xFF)
  {
      // No devices on this controller
      return;
  }

  // Create two disks - master and slave.
  AtaDisk *pMaster = new AtaDisk(this, true);
  AtaDisk *pSlave = new AtaDisk(this, false);

  // Try and initialise the disks.
  bool masterInitialised = pMaster->initialise();
  bool slaveInitialised = pSlave->initialise();

  NOTICE("AtaController installing IRQ " << getInterruptNumber() << ".");
  Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler*> (this));

  // Initialise potential ATAPI disks (add as children before initialising so they get IRQs)
  if (masterInitialised)
    addChild(pMaster);
  else
  {
    delete pMaster;
    AtapiDisk *pMasterAtapi = new AtapiDisk(this, true);
    addChild(pMasterAtapi);
    if(!pMasterAtapi->initialise())
    {
      removeChild(pMasterAtapi);
      delete pMasterAtapi;
    }
  }

  if (slaveInitialised)
    addChild(pSlave);
  else
  {
    delete pSlave;
    AtapiDisk *pSlaveAtapi = new AtapiDisk(this, false);
    addChild(pSlaveAtapi);
    if(!pSlaveAtapi->initialise())
    {
      removeChild(pSlaveAtapi);
      delete pSlaveAtapi;
    }
  }

  initialise();

}

AtaController::~AtaController()
{
}

uint64_t AtaController::executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
                                       uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8)
{
  AtaDisk *pDisk = reinterpret_cast<AtaDisk*> (p2);
  if(p1 == ATA_CMD_READ)
    return pDisk->doRead(p3);
  else if(p1 == ATA_CMD_WRITE)
    return pDisk->doWrite(p3);
  else
    return 0;
}

bool AtaController::irq(irq_id_t number, InterruptState &state)
{
  for (unsigned int i = 0; i < getNumChildren(); i++)
  {
    AtaDisk *pDisk = static_cast<AtaDisk*> (getChild(i));
    if(pDisk->isAtapi())
    {
        AtapiDisk *pAtapiDisk = static_cast<AtapiDisk*>(pDisk);
        pAtapiDisk->irqReceived();
    }
    else
        pDisk->irqReceived();
  }
  return false; // Keep the IRQ disabled - level triggered.
}

#endif
