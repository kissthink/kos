#include "mach/DMA.h"
#include "kern/Debug.h"

const BitSeg<uint8_t, 0, 1> DMA::Sel0;
const BitSeg<uint8_t, 1, 1> DMA::Sel1;
const BitSeg<uint8_t, 2, 1> DMA::MaskOn;

const BitSeg<uint8_t, 0, 1> DMA::Mask0;
const BitSeg<uint8_t, 1, 1> DMA::Mask1;
const BitSeg<uint8_t, 2, 1> DMA::Mask2;
const BitSeg<uint8_t, 3, 1> DMA::Mask3;

const BitSeg<uint8_t, 2, 1> DMA::Transfer0;
const BitSeg<uint8_t, 3, 1> DMA::Transfer1;
const BitSeg<uint8_t, 4, 1> DMA::Auto;
const BitSeg<uint8_t, 5, 1> DMA::Down;
const BitSeg<uint8_t, 6, 1> DMA::Mode0;
const BitSeg<uint8_t, 7, 1> DMA::Mode1;

const BitSeg<uint8_t, 0, 1> DMA::TransferComplete0;
const BitSeg<uint8_t, 1, 1> DMA::TransferComplete1;
const BitSeg<uint8_t, 2, 1> DMA::TransferComplete2;
const BitSeg<uint8_t, 3, 1> DMA::TransferComplete3;
const BitSeg<uint8_t, 4, 1> DMA::RequestPending0;
const BitSeg<uint8_t, 5, 1> DMA::RequestPending1;
const BitSeg<uint8_t, 6, 1> DMA::RequestPending2;
const BitSeg<uint8_t, 7, 1> DMA::RequestPending3;

const BitSeg<uint8_t, 0, 1> DMA::MMT;   // does not work
const BitSeg<uint8_t, 1, 1> DMA::ADHE;  // does not work
const BitSeg<uint8_t, 2, 1> DMA::COND;  // disables DMA controller
const BitSeg<uint8_t, 3, 1> DMA::COMP;  // does not work
const BitSeg<uint8_t, 4, 1> DMA::PRIO;  // does not work
const BitSeg<uint8_t, 5, 1> DMA::EXTW;  // does not work
const BitSeg<uint8_t, 6, 1> DMA::DRQP;
const BitSeg<uint8_t, 7, 1> DMA::DACKP;

void DMA::resetFF(uint8_t channel) {
  if (channel < 4) out8( static_cast<uint8_t>(SlaveRegisters::FFReset), 0xff );
  else out8( static_cast<uint8_t>(MasterRegisters::FFReset), 0xff );
}

void DMA::maskDRQ(uint8_t channel, bool mask) {
  if (channel < 4) out8( static_cast<uint8_t>(SlaveRegisters::SingleChannelMask), (channel & (Sel0()|Sel1())) | MaskOn(mask) );
  else out8( static_cast<uint8_t>(MasterRegisters::SingleChannelMask), (channel & (Sel0()|Sel1())) | MaskOn(mask) );
}

uint8_t DMA::getStartAddress(uint8_t channel) {
  switch (channel) {
    case 0: return static_cast<uint8_t>(Channel0::StartAddress);
    case 1: return static_cast<uint8_t>(Channel1::StartAddress);
    case 2: return static_cast<uint8_t>(Channel2::StartAddress);
    case 3: return static_cast<uint8_t>(Channel3::StartAddress);
    case 4: return static_cast<uint8_t>(Channel4::StartAddress);
    case 5: return static_cast<uint8_t>(Channel5::StartAddress);
    case 6: return static_cast<uint8_t>(Channel6::StartAddress);
    case 7: return static_cast<uint8_t>(Channel7::StartAddress);
    default: break; // error caught above
  }
  return 0;
}

uint8_t DMA::getCount(uint8_t channel) {
  switch (channel) {
    case 0: return static_cast<uint8_t>(Channel0::Count);
    case 1: return static_cast<uint8_t>(Channel1::Count);
    case 2: return static_cast<uint8_t>(Channel2::Count);
    case 3: return static_cast<uint8_t>(Channel3::Count);
    case 4: return static_cast<uint8_t>(Channel4::Count);
    case 5: return static_cast<uint8_t>(Channel5::Count);
    case 6: return static_cast<uint8_t>(Channel6::Count);
    case 7: return static_cast<uint8_t>(Channel7::Count);
    default: break; // error caught above
  }
  return 0;
}

uint8_t DMA::getPageAddress(uint8_t channel) {
  switch (channel) {
    case 0: return static_cast<uint8_t>(Channel0::PageAddress);
    case 1: return static_cast<uint8_t>(Channel1::PageAddress);
    case 2: return static_cast<uint8_t>(Channel2::PageAddress);
    case 3: return static_cast<uint8_t>(Channel3::PageAddress);
    case 4: return static_cast<uint8_t>(Channel4::PageAddress);
    case 5: return static_cast<uint8_t>(Channel5::PageAddress);
    case 6: return static_cast<uint8_t>(Channel6::PageAddress);
    case 7: return static_cast<uint8_t>(Channel7::PageAddress);
    default: break; // error caught above
  }
  return 0;
}

bool DMA::setupTransfer(uint8_t channel, uint8_t mode, size_t length, laddr buffer) {
  if (channel == 0 || channel == 4) return false;   // unusable channels
  if (length >= 0x10000) return false;              // transfer cannot be bigger than 64kb

  maskDRQ(channel); // prevent DMA request (DRQ) arriving during setting up DMA controller
  resetFF(channel); // make sure the flip-flop is in proper state
  out8( getStartAddress(channel), buffer & 0xff );
  out8( getStartAddress(channel), (buffer >> 8) & 0xff );  // write in 8-bit at a time

  resetFF(channel);
  out8( getCount(channel), length & 0xff );
  out8( getCount(channel), (length >> 8) & 0xff );

  uint8_t externalPageRegister = buffer / 0x10000;
  out8( getPageAddress(channel), externalPageRegister );
  maskDRQ(channel, false);  // unmask

  return true;
}

void DMA::setDMAMode(uint8_t channel, uint8_t mode, bool setAuto) {
  uint8_t modeRegister = 0;
  modeRegister |= (channel & (Sel0()|Sel1()));
  modeRegister |= (mode & (Transfer0()|Transfer1()|Auto(setAuto)|Mode0()|Mode1()));
  if (channel < 4)  out8( static_cast<uint8_t>(SlaveRegisters::Mode), modeRegister );
  else out8( static_cast<uint8_t>(MasterRegisters::Mode), modeRegister );
}

bool DMA::startTransfer(uint8_t channel, uint8_t mode, size_t length, laddr buffer) {
  if (!setupTransfer(channel, mode, length-1, buffer)) return false;
  maskDRQ(channel);         // mask DRQ until command is sent
  setDMAMode(channel, mode);
  maskDRQ(channel, false);  // unmask
  return true;
}
