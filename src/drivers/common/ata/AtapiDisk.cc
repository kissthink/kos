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
 * WHATSOEVER RESULTING FROM LOSS OF USE, DAtapi OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "mach/Device.h"
#include "mach/Disk.h"
#include "mach/Controller.h"
#include "kern/Debug.h"
#include "util/endian.h"
#include "drivers/common/ata/AtaController.h"
#include "drivers/common/ata/BusMasterIde.h"
#include "drivers/common/ata/AtapiDisk.h"
#include "drivers/common/ata/ata-common.h"

/// \todo Make portable
/// \todo GET MEDIA STATUS to find out if there's actually media!

// Note the IrqReceived mutex is deliberately started in the locked state.
AtapiDisk::AtapiDisk(AtaController *pDev, bool isMaster, IOBase *commandRegs, IOBase *controlRegs, BusMasterIde *busMaster) :
  AtaDisk(pDev, isMaster, commandRegs, controlRegs, busMaster), m_IsMaster(isMaster), m_SupportsLBA28(true),
  m_SupportsLBA48(false), m_Type(None), m_NumBlocks(0), m_BlockSize(0), m_PacketSize(0), m_Removable(true), m_IrqReceived(),
  m_PrdTableMemRegion("atapi-prdtable")
{
  m_pParent = pDev;
  m_IrqReceived.acquire();
}

AtapiDisk::~AtapiDisk()
{
}

bool AtapiDisk::initialise()
{
  // Grab our parent.
  AtaController *pParent = static_cast<AtaController*> (m_pParent);

  // Grab our parent's IoPorts for command and control accesses.
  IOBase *commandRegs = m_CommandRegs;
  // Commented out - unused variable.
  IOBase *controlRegs = m_ControlRegs;

  // Drive spin-up (go from standby to active, if necessary)
  setFeatures(0x07, 0, 0, 0, 0);

  // Check for device presence
  uint8_t devSelect = (m_IsMaster) ? 0xA0 : 0xB0;
  commandRegs->write8(devSelect, 6);
  commandRegs->write8(0xA1, 7);
  if(commandRegs->read8(7) == 0)
  {
    DBG::outln(DBG::Drivers, "ATAPI: No device present here");
    return false;
  }

  // Select the device to transmit to
  devSelect = (m_IsMaster) ? 0xA0 : 0xB0;
  commandRegs->write8(devSelect, 6);

  // Wait for it to be selected
  ataWait(commandRegs);

  // DEVICE RESET
  commandRegs->write8(8, 7);

  // Wait for the drive to reset before requesting a device change
  ataWait(commandRegs);

  //
  // Start IDENTIFY command.
  //

  // Send IDENTIFY.
  uint8_t status = commandRegs->read8(7);
  commandRegs->write8(0xEC, 7);

  // Read status register.
  status = commandRegs->read8(7);

  if (status == 0)
    // Device does not exist.
    return false;

  // Poll until BSY is clear and either ERR or DRQ are set
  while((status&0x80) || !(status&0x9))
    status = commandRegs->read8(7);

  // Check for an ATAPI device
  uint8_t m1 = commandRegs->read8(2);
  uint8_t m2 = commandRegs->read8(3);
  uint8_t m3 = commandRegs->read8(4);
  uint8_t m4 = commandRegs->read8(5);
  DBG::outln(DBG::Drivers, "ATA 'magic registers': ", FmtHex(m1), ", ", FmtHex(m2), ", ", FmtHex(m3), ", ", FmtHex(m4));

  if(m1 == 0x01 && m2 == 0x01 && m3 == 0x14 && m4 == 0xeb)
  {
    // Run IDENTIFY PACKET DEVICE instead
    commandRegs->write8( (m_IsMaster)?0xA0:0xB0, 6 );
    commandRegs->write8(0xA1, 7);
    status = commandRegs->read8(7);

    // Poll until BSY is clear and either ERR or DRQ are set
    while((status&0x80) || !(status&0x9))
      status = commandRegs->read8(7);
  }
  else
  {
    if(m3 == 0x14 && m4 == 0xeb)
    {
      // Run IDENTIFY PACKET DEVICE instead
      commandRegs->write8((m_IsMaster)?0xA0:0xB0, 6 );
      commandRegs->write8(0xA1, 7);
      status = commandRegs->read8(7);

      // Poll until BSY is clear and either ERR or DRQ are set
      while((status&0x80) || !(status&0x9))
        status = commandRegs->read8(7);
    }
    else
    {
        DBG::outln(DBG::Drivers, "NON-ATAPI device, skipping");
        return false;
    }
  }

  // If ERR was set we had an err0r.
  if (status & 0x1)
  {
    DBG::outln(DBG::Drivers, "ATAPI drive errored on IDENTIFY [status=", status, "]!");
    return false;
  }

  // Read the data.
  for (int i = 0; i < 256; i++)
  {
    m_pIdent[i] = commandRegs->read16(0);
  }

  status = commandRegs->read8(7);

  // Interpret the data.

  // Get the device name.
  for (int i = 0; i < 20; i++)
  {
    m_pName[i*2] = m_pIdent[0x1B+i] >> 8;
    m_pName[i*2+1] = m_pIdent[0x1B+i] & 0xFF;
#if 0 // big-endian
    m_pName[i*2] = m_pIdent[0x1B+i] & 0xFF;
    m_pName[i*2+1] = m_pIdent[0x1B+i] >> 8;
#endif
  }

  // The device name is padded by spaces. Backtrack through converting spaces into NULL bytes.
  for (int i = 39; i > 0; i--)
  {
    if (m_pName[i] != ' ')
      break;
    m_pName[i] = '\0';
  }
  m_pName[40] = '\0';


  // Get the serial number.
  for (int i = 0; i < 10; i++)
  {
    m_pSerialNumber[i*2] = m_pIdent[0x0A+i] >> 8;
    m_pSerialNumber[i*2+1] = m_pIdent[0x0A+i] & 0xFF;
#if 0 // big-endian
    m_pSerialNumber[i*2] = m_pIdent[0x0A+i] & 0xFF;
    m_pSerialNumber[i*2+1] = m_pIdent[0x0A+i] >> 8;
#endif
  }

  // The serial number is padded by spaces. Backtrack through converting spaces into NULL bytes.
  for (int i = 19; i > 0; i--)
  {
    if (m_pSerialNumber[i] != ' ')
      break;
    m_pSerialNumber[i] = '\0';
  }
  m_pSerialNumber[20] = '\0';

  // Get the firmware revision.
  for (int i = 0; i < 4; i++)
  {
    m_pFirmwareRevision[i*2] = m_pIdent[0x17+i] >> 8;
    m_pFirmwareRevision[i*2+1] = m_pIdent[0x17+i] & 0xFF;
#if 0 // big-endian
    m_pFirmwareRevision[i*2] = m_pIdent[0x17+i] & 0xFF;
    m_pFirmwareRevision[i*2+1] = m_pIdent[0x17+i] >> 8;
#endif
  }

  // The device name is padded by spaces. Backtrack through converting spaces into NULL bytes.
  for (int i = 7; i > 0; i--)
  {
    if (m_pFirmwareRevision[i] != ' ')
      break;
    m_pFirmwareRevision[i] = '\0';
  }
  m_pFirmwareRevision[8] = '\0';

  uint16_t word83 = LITTLE_TO_HOST16(m_pIdent[83]);
  if (word83 & (1<<10))
  {
    m_SupportsLBA48 = /*true*/ false;
  }

  // Any form of DMA support?
  if(!(m_pIdent[49] & (1 << 8)))
  {
      DBG::outln(DBG::Drivers, "ATAPI: Device does not support DMA");
      m_bDma = false;
  }
  else if(!m_BusMaster)
  {
      DBG::outln(DBG::Drivers, "ATAPI: Controller does not support DMA");
      m_bDma = false;
  }

  // Packet size?
  m_PacketSize = ((m_pIdent[0] & 0x0003) == 0) ? 12 : 16;

  // Check that there's actually media to use. If not, we bail...
  /// \todo Support inserting and ejecting media at runtime
  commandRegs->write8(0xDA, 7); // GET MEDIA STATUS
  status = commandRegs->read8(7);
  while((status&0x80) || !(status&0x9))
    status = commandRegs->read8(7);
  if(status & 0x1)
  {
      // We have information in the error register
      uint8_t err = commandRegs->read8(1);

      // ABORT?
      if(err & 0x4)
      {
          DBG::outln(DBG::Drivers, "ATAPI: GET MEDIA STATUS command aborted by device");
          return false;
      }
      else if(err & 2)
      {
          DBG::outln(DBG::Drivers, "ATAPI: No media present in the drive - aborting.");
          DBG::outln(DBG::Drivers, "       TODO: handle media changes/insertions/removal properly");
          return false;
      }
      else
      {
          DBG::outln(DBG::Drivers, "ATAPI: Media status: ", err, ".");
      }
  }

  // Grab the capacity of the disk for future reference
  if(!getCapacityInternal(&m_NumBlocks, &m_BlockSize))
  {
    DBG::outln(DBG::Drivers, "Couldn't get internal capacity on ATAPI device!");
    return false;
  }

  // Send an INQUIRY command to find more information about the disk
  Inquiry inquiry;
  struct InquiryCommand
  {
    uint8_t Opcode;
    uint8_t LunEvpd;
    uint8_t PageCode;
    uint16_t AllocLen;
    uint8_t Control;
  } __attribute__((packed)) inqCommand;
  memset(&inqCommand, 0, sizeof(InquiryCommand));
  inqCommand.Opcode = 0x12; // INQUIRY command
  inqCommand.AllocLen = HOST_TO_BIG16(sizeof(Inquiry));
  bool success = sendCommand(sizeof(Inquiry), reinterpret_cast<uintptr_t>(&inquiry), sizeof(InquiryCommand), reinterpret_cast<uintptr_t>(&inqCommand));
  if(!success)
  {
    // Uh oh!
    DBG::outln(DBG::Drivers, "Couldn't run INQUIRY command on ATAPI device!");
    return false;
  }

  // Removable - and get the type too
  m_Removable = ((inquiry.Removable & (1 << 7)) != 0);
  m_Type = static_cast<AtapiType>(inquiry.Peripheral);

  // Supported device?
  if(m_Type != CdDvd && m_Type != Block)
  {
    /// \todo Testing needs to be done on more than just CD/DVD and block devices...
    DBG::outln(DBG::Drivers, "KOS currently only supports CD/DVD and block ATAPI devices.");
    return false;
  }

  DBG::outln(DBG::Drivers, "Detected ATAPI device '", m_pName, "', '", m_pSerialNumber, "', '", m_pFirmwareRevision, "'");
  return true;
}

uint64_t AtapiDisk::doRead(uint64_t location)
{
    size_t off = location & 0xFFF;
    location &= ~0xFFF;
    uintptr_t buffer = m_Cache.lookup(location);
    if(!buffer)
    {
        buffer = m_Cache.insert(location);
        doRead2(location, buffer, 4096);
    }
    return buffer + off;
}

uint64_t AtapiDisk::doRead2(uint64_t location, uintptr_t buffer, size_t buffSize)
{
    size_t nBytes = buffSize;

    if(!nBytes || !buffer)
    {
        DBG::outln(DBG::Drivers, "Bad arguments to AtapiDisk::doRead");
        return 0;
    }

    if(location > (m_BlockSize * m_NumBlocks))
    {
        DBG::outln(DBG::Drivers, "ATAPI: Attempted read beyond disk size");
        return 0;
    }

    size_t blockNum = location / m_BlockSize;
    size_t numBlocks = nBytes / m_BlockSize;
    if(nBytes & m_BlockSize)
        numBlocks++;

    if(m_Type == CdDvd)
    {
        // Read the TOC in order to get the data track.
        // This is added to the block number to read the data
        // off the disk properly.
        struct ReadTocCommand
        {
            uint8_t Opcode;
            uint8_t Flags;
            uint8_t Format;
            uint8_t Rsvd[3];
            uint8_t Track;
            uint16_t AllocLen;
            uint8_t Control;
        } __attribute__((packed)) tocCommand;
        memset(&tocCommand, 0, sizeof(ReadTocCommand));
        tocCommand.Opcode = 0x43; // READ TOC command
        tocCommand.AllocLen = HOST_TO_BIG16(m_BlockSize);

        uint8_t *tmpBuff = new uint8_t[m_BlockSize];
        PointerGuard<uint8_t> tmpBuffGuard(tmpBuff);
        bool success = sendCommand(m_BlockSize, reinterpret_cast<uintptr_t>(tmpBuff), sizeof(ReadTocCommand), reinterpret_cast<uintptr_t>(&tocCommand));
        if(!success)
        {
            DBG::outln(DBG::Drivers, "Could not read the TOC!");
            return 0;
        }

        uint16_t i;
        bool bHaveTrack = false;
        uint16_t bufLen = BIG_TO_HOST16(*reinterpret_cast<uint16_t*>(tmpBuff));
        TocEntry *Toc = reinterpret_cast<TocEntry*>(tmpBuff + 4);
        for(i = 0; i < (bufLen / 8); i++)
        {
            if(Toc[i].Flags == 0x14)
            {
                bHaveTrack = true;
                break;
            }
        }

        if(!bHaveTrack)
        {
            DBG::outln(DBG::Drivers, "ATAPI: CD does not have a data track!");
            return 0;
        }

        uint32_t trackStart = BIG_TO_HOST32(Toc[i].TrackStart); // Start of the track, LBA
        if((blockNum + trackStart) < blockNum)
        {
            DBG::outln(DBG::Drivers, "ATAPI TOC overflow");
            return 0;
        }

        blockNum += trackStart;
    }

    // Read the data now
    struct ReadCommand
    {
        uint8_t Opcode;
        uint8_t Flags;
        uint32_t StartLBA;
        uint8_t Rsvd;
        uint16_t BlockCount;
        uint8_t Control;
    } __attribute__((packed)) command;
    memset(&command, 0, sizeof(ReadCommand));
    command.Opcode = 0x28; // READ(10) command
    command.StartLBA = HOST_TO_BIG32(blockNum);
    command.BlockCount = HOST_TO_BIG16(numBlocks);

    bool success = sendCommand(nBytes, buffer, sizeof(ReadCommand), reinterpret_cast<uintptr_t>(&command));
    if(!success)
    {
        DBG::outln(DBG::Drivers, "Read command failed on an ATAPI device");
        return 0;
    }

    return nBytes;
}

uint64_t AtapiDisk::doWrite(uint64_t location)
{
  return 0;
}

void AtapiDisk::irqReceived()
{
  m_IrqReceived.release();
}

bool AtapiDisk::sendCommand(size_t nRespBytes, uintptr_t respBuff, size_t nPackBytes, uintptr_t packet, bool bWrite)
{
  if(!m_PacketSize)
  {
    DBG::outln(DBG::Drivers, "sendCommand called but the packet size is not known!");
    return false;
  }

  AtaStatus status;

  // Grab our parent.
  AtaController *pParent = static_cast<AtaController*> (m_pParent);

  // Grab our parent's IoPorts for command and control accesses.
  IOBase *commandRegs = m_CommandRegs;
  IOBase *controlRegs = m_ControlRegs;

  // Temporary storage, so we can save cycles later
  uint16_t *tmpPacket = new uint16_t[m_PacketSize / 2];
  PointerGuard<uint16_t> tmpGuard(tmpPacket);
  memcpy(tmpPacket, reinterpret_cast<void*>(packet), nPackBytes);
  memset(tmpPacket + (nPackBytes / 2), 0, m_PacketSize - nPackBytes);

  // Round up the packet size to the nearest even number
  if(nPackBytes & 0x1)
    nPackBytes++;

  // Wait for the device to finish any outstanding operations
  status = ataWait(commandRegs);

  // Select the device to transmit to
  uint8_t devSelect = (m_IsMaster) ? 0xA0 : 0xB0;
  commandRegs->write8(devSelect, 6);

  // Wait for it to be selected
  status = ataWait(commandRegs);

  // Verify that it's the correct device
  if(commandRegs->read8(6) != devSelect)
  {
      DBG::outln(DBG::Drivers, "ATAPI: Device was not selected");
      return false;
  }

  // PACKET command
  if((m_pIdent[62] & (1 << 15)) && m_bDma) // Device requires DMADIR for Packet DMA commands
      commandRegs->write8((bWrite ? 1 : 5), 1); // Transfer to host, DMA
  else if(m_bDma)
      commandRegs->write8(1, 1); // No overlap, DMA
  else
    commandRegs->write8(0, 1); // No overlap, no DMA
  commandRegs->write8(0, 2); // Tag = 0
  commandRegs->write8(0, 3); // N/A for PACKET command
  if(m_bDma)
  {
      commandRegs->write8(0, 4); // Byte count limit = 0 for DMA
      commandRegs->write8(0, 5);
  }
  else
  {
      commandRegs->write8(nRespBytes & 0xFF, 4); // Byte count limit
      commandRegs->write8(((nRespBytes >> 8) & 0xFF), 5);
  }
  commandRegs->write8(0xA0, 7);

  // Wait for the device to be ready to accept the command packet
#if 0
  status = commandRegs->read8(7);
  while((status&0x80) || !(status&0x9))
         /*
         ! // This is "right" according to the spec, but it makes QEMU die :(
         ((status & 0x80) == 0) && // BSY cleared to zero
         ((status & 0x20) == 0) && // DMRD cleared to zero
         ((status & 0x08) == 1) && // DRQ set to one
         ((status & 0x01) == 0)))  // CHK cleared to zero
         */
  {
      status = commandRegs->read8(7);
  }
#endif
  status = ataWait(commandRegs);

  // Error?
  if(status.reg.err)
  {
    DBG::outln(DBG::Drivers, "ATAPI Packet command error [status=", FmtHex(status.__reg_contents), "]!");
    return false;
  }

  // If DMA is enabled, set that up now
  bool bDmaSetup = false;
  if(m_bDma && nRespBytes)
  {
      bDmaSetup = m_BusMaster->add(respBuff, nRespBytes);
      if(bDmaSetup)
        bDmaSetup = m_BusMaster->begin(bWrite);
  }

  // Ensure interrupts are actually enabled now.
  /// \todo Find the module that's being naughty and disabling interrupts
  bool oldInterrupts = Processor::interruptsEnabled();
  if (!oldInterrupts) Processor::enableInterrupts();

  Machine::instance().getIrqManager()->enable(getParent()->getInterruptNumber(), true);

  // Transmit the command (padded as needed)
  for(size_t i = 0; i < (m_PacketSize / 2); i++)
    commandRegs->write16(tmpPacket[i], 0);

  // Wait for the busy bit to be cleared once again
  while(true)
  {
    m_IrqReceived.acquire();
    if(m_bDma && bDmaSetup)
    {
        if(m_BusMaster->hasInterrupt())
        {
            // commandComplete effectively resets the device state, so we need
            // to get the error register first.
            bool bError = m_BusMaster->hasError();
            m_BusMaster->commandComplete();
            if(bError)
                return 0;
            else
                break;
        }
    }
    else
        break;
  }
  if (!oldInterrupts) Processor::disableInterrupts();
  
  status = ataWait(commandRegs);

  if(status.reg.err)
  {
    DBG::outln(DBG::Drivers, "ATAPI sendCommand failed after sending command packet [status=", FmtHex(status.__reg_contents), "]");
    return false;
  }

  // Check for DRQ, if not set, there's nothing to read
  if(!status.reg.drq)
    return true;

  // Read in the data, if we need to
  if(!m_bDma && !bDmaSetup)
  {
      size_t realSz = commandRegs->read8(4) | (commandRegs->read8(5) << 8);
      uint16_t *dest = reinterpret_cast<uint16_t*>(respBuff);
      if(nRespBytes)
      {
        size_t sizeToRead = ((realSz > nRespBytes) ? nRespBytes : realSz) / 2;
        for(size_t i = 0; i < sizeToRead; i++)
        {
          if(bWrite)
            commandRegs->write16(dest[i], 0);
          else
            dest[i] = commandRegs->read16(0);
        }
      }

      // Discard unread data (or write pretend data)
      if(realSz > nRespBytes)
      {
        DBG::outln(DBG::Drivers, "sendCommand has to read beyond provided buffer [", FmtHex(realSz), " is bigger than ", FmtHex(nRespBytes), "]");
        for(size_t i = nRespBytes; i < realSz; i += 2)
        {
          if(bWrite)
            commandRegs->write16(0xFFFF, 0);
          else
            commandRegs->read16(0);
        }
      }
  }

  // Complete
  uint8_t endStatus = commandRegs->read8(7);
  return (!(endStatus & 0x01));
}

bool AtapiDisk::readSense(uintptr_t buff)
{
  // Convert the passed buffer
  Sense *sense = reinterpret_cast<Sense*>(buff);
  memset(sense, 0xFF, sizeof(Sense));

  // Command
  struct SenseCommand
  {
    uint8_t Opcode;
    uint8_t Desc;
    uint8_t Rsvd[2];
    uint8_t AllocLength;
    uint8_t Control;
  } __attribute__((packed)) senseComm;
  memset(&senseComm, 0, sizeof(SenseCommand));
  senseComm.Opcode = 0x03; // REQUEST SENSE
  senseComm.AllocLength = sizeof(Sense);

  bool success = sendCommand(sizeof(Sense), buff, sizeof(SenseCommand), reinterpret_cast<uintptr_t>(&senseComm));
  if(!success)
  {
    DBG::outln(DBG::Drivers, "ATAPI: SENSE command failed");
    return false;
  }

  return ((sense->ResponseCode & 0x70) == 0x70);
}

bool AtapiDisk::unitReady()
{
  // Command
  Sense sense;
  struct UnitReadyCommand
  {
    uint8_t Opcode;
    uint8_t Rsvd[4];
    uint8_t Control;
  } __attribute__((packed)) command;
  memset(&command, 0, sizeof(UnitReadyCommand));
  command.Opcode = 0x00; // TEST UNIT READY

  bool success = false;
  int retry = 5;
  do
  {
    success = sendCommand(0, 0, sizeof(UnitReadyCommand), reinterpret_cast<uintptr_t>(&command));
  } while((success == false) && readSense(reinterpret_cast<uintptr_t>(&sense)) && --retry);

  if(m_Removable &&
      ((sense.ResponseCode & 0x70) == 0x70) &&
      ((sense.SenseKey == 0x06) || (sense.SenseKey == 0x02)) /* UNIT_ATTN or NOT_READY */
    )
  {
    success = true;
  }

  return success;
}

bool AtapiDisk::getCapacityInternal(size_t *blockNumber, size_t *blockSize)
{
  if(!unitReady())
  {
    *blockNumber = 0;
    *blockSize = defaultBlockSize();
    return false;
  }

  struct Capacity
  {
    uint32_t LBA;
    uint32_t BlockSize;
  } __attribute__((packed)) capacity;
  memset(&capacity, 0, sizeof(Capacity));

  struct CapacityCommand
  {
    uint8_t Opcode;
    uint8_t LunRelAddr;
    uint32_t LBA;
    uint8_t Rsvd[2];
    uint8_t PMI;
    uint8_t Control;
  } __attribute__((packed)) command;
  memset(&command, 0, sizeof(CapacityCommand));
  command.Opcode = 0x25; // READ CAPACITY
  bool success = sendCommand(sizeof(Capacity), reinterpret_cast<uintptr_t>(&capacity), sizeof(CapacityCommand), reinterpret_cast<uintptr_t>(&command));
  if(!success)
  {
    DBG::outln(DBG::Drivers, "ATAPI READ CAPACITY command failed");
    return false;
  }

  *blockNumber = BIG_TO_HOST32(capacity.LBA);
  uint32_t blockSz = BIG_TO_HOST32(capacity.BlockSize);
  *blockSize = blockSz ? blockSz : defaultBlockSize();

  return true;
}
