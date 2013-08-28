#ifndef _DMA_h_
#define _DMA_h_ 1

#include "mach/platform.h"
#include "util/basics.h"

// ISA DMA  http://wiki.osdev.org/DMA

class DMA {
  DMA() = delete;
  DMA(const DMA&) = delete;
  DMA& operator=(const DMA&) = delete;

  enum class SlaveRegisters : uint8_t {
    Status            = 0x08,   // R
    Command           = 0x08,   // W
    Request           = 0x09,   // W
    SingleChannelMask = 0x0a,   // W
    Mode              = 0x0b,   // W
    FFReset           = 0x0c,   // W
    Intermediate      = 0x0d,   // R
    MasterReset       = 0x0d,   // W
    MaskReset         = 0x0e,   // W
    MultiChannelMask  = 0x0f    // RW
  };

  enum class MasterRegisters : uint8_t {
    Status            = 0xd0,   // R
    Command           = 0xd0,   // W
    Request           = 0xd2,   // W
    SingleChannelMask = 0xd4,   // W
    Mode              = 0xd6,   // W
    FFReset           = 0xd8,   // W
    Intermediate      = 0xda,   // R
    MasterReset       = 0xda,   // W
    MaskReset         = 0xdc,   // W
    MultiChannelMask  = 0xde    // RW
  };

  // SingleChannelMask
  static const BitSeg<uint8_t, 0, 1> Sel0;
  static const BitSeg<uint8_t, 1, 1> Sel1;
  static const BitSeg<uint8_t, 2, 1> MaskOn;

  // MultiChannelMask
  static const BitSeg<uint8_t, 0, 1> Mask0;
  static const BitSeg<uint8_t, 1, 1> Mask1;
  static const BitSeg<uint8_t, 2, 1> Mask2;
  static const BitSeg<uint8_t, 3, 1> Mask3;

  // Mode
//  static const BitSeg<uint8_t, 0, 1> Sel0;
//  static const BitSeg<uint8_t, 1, 1> Sel1;
  static const BitSeg<uint8_t, 2, 1> Transfer0;
  static const BitSeg<uint8_t, 3, 1> Transfer1;
  static const BitSeg<uint8_t, 4, 1> Auto;
  static const BitSeg<uint8_t, 5, 1> Down;
  static const BitSeg<uint8_t, 6, 1> Mode0;
  static const BitSeg<uint8_t, 7, 1> Mode1;

  enum class TransferType {
    SelfTest = 0b00,
    Write    = 0b01,
    Read     = 0b10,
    Invalid  = 0b11
  };

  enum class TransferMode {
    OnDemand = 0b00,
    Single   = 0b01,
    Block    = 0b10,
    Cascade  = 0b11
  };

  // Status
  static const BitSeg<uint8_t, 0, 1> TransferComplete0;
  static const BitSeg<uint8_t, 1, 1> TransferComplete1;
  static const BitSeg<uint8_t, 2, 1> TransferComplete2;
  static const BitSeg<uint8_t, 3, 1> TransferComplete3;
  static const BitSeg<uint8_t, 4, 1> RequestPending0;
  static const BitSeg<uint8_t, 5, 1> RequestPending1;
  static const BitSeg<uint8_t, 6, 1> RequestPending2;
  static const BitSeg<uint8_t, 7, 1> RequestPending3;

  // Command
  static const BitSeg<uint8_t, 0, 1> MMT;   // does not work
  static const BitSeg<uint8_t, 1, 1> ADHE;  // does not work
  static const BitSeg<uint8_t, 2, 1> COND;  // disables DMA controller
  static const BitSeg<uint8_t, 3, 1> COMP;  // does not work
  static const BitSeg<uint8_t, 4, 1> PRIO;  // does not work
  static const BitSeg<uint8_t, 5, 1> EXTW;  // does not work
  static const BitSeg<uint8_t, 6, 1> DRQP;
  static const BitSeg<uint8_t, 7, 1> DACKP;

  enum class Channel0 : uint8_t {
    StartAddress      = 0x00,   // W
    Count             = 0x01,   // W
    PageAddress       = 0x87    // unstable
  };
  enum class Channel1 : uint8_t {
    StartAddress      = 0x02,   // W
    Count             = 0x03,   // W
    PageAddress       = 0x83
  };
  enum class Channel2 : uint8_t {
    StartAddress      = 0x04,   // W
    Count             = 0x05,   // W
    PageAddress       = 0x81
  };
  enum class Channel3 : uint8_t {
    StartAddress      = 0x06,   // W
    Count             = 0x07,   // W
    PageAddress       = 0x82
  };
  enum class Channel4 : uint8_t {
    StartAddress      = 0xc0,   // W
    Count             = 0xc2,   // W
    PageAddress       = 0x8f    // unstable
  };
  enum class Channel5 : uint8_t {
    StartAddress      = 0xc4,   // W
    Count             = 0xc6,   // W
    PageAddress       = 0x8b
  };
  enum class Channel6 : uint8_t {
    StartAddress      = 0xc8,   // W
    Count             = 0xca,   // W
    PageAddress       = 0x89
  };
  enum class Channel7 : uint8_t {
    StartAddress      = 0xcc,   // W
    Count             = 0xce,   // W
    PageAddress       = 0x8a
  };

  static void resetFF(uint8_t channel);
  static void maskDRQ(uint8_t channel, bool mask = true);
  static void setDMAMode(uint8_t channel, uint8_t mode, bool setAuto = false);
  static uint8_t getStartAddress(uint8_t channel);
  static uint8_t getCount(uint8_t channel);
  static uint8_t getPageAddress(uint8_t channel);
  static bool setupTransfer(uint8_t channel, uint8_t mode, size_t length, laddr buffer);
public:
  static bool startTransfer(uint8_t channel, uint8_t mode, size_t length, laddr buffer);

};

#endif /* _DMA_h_ */
