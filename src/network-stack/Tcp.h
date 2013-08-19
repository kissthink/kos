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
#ifndef _NetworkStack_TCP_h_
#define _NetworkStack_TCP_h_ 1

#include "util/String.h"
#include "util/Vector.h"
#include "util/basics.h"
#include "ipc/BlockingSync.h"
#include "mach/Network.h"

#include "network-stack/IpCommon.h"

/**
 * The Pedigree network stack - TCP layer
 */
class Tcp
{
private:

  static Tcp tcpInstance;

	struct tcpOption
	{
		uint8_t optkind;
		uint8_t optlen;
	} __attribute__((packed));

  // Psuedo-header for checksum when being sent over IPv6
  struct tcpPsuedoHeaderIpv6
  {
    uint8_t  src_addr[16];
    uint8_t  dest_addr[16];
    uint32_t length;
    uint8_t  nextHeader;
    uint16_t zero1;
    uint8_t  zero2;
  } __attribute__ ((packed));

public:
  Tcp();
  virtual ~Tcp();

  /** For access to the stack without declaring an instance of it */
  static Tcp& instance()
  {
    return tcpInstance;
  }

  struct tcpHeader
  {
    uint16_t  src_port;
    uint16_t  dest_port;
    uint32_t  seqnum;
    uint32_t  acknum;
    uint32_t  rsvd : 4;
    uint32_t  offset : 4;
    uint8_t   flags;
    uint16_t  winsize;
    uint16_t  checksum;
    uint16_t  urgptr;
  } __attribute__ ((packed));

  /** Packet arrival callback */
  void receive(IpAddress from, IpAddress to, uintptr_t packet, size_t nBytes, IpBase *pIp, Network* pCard);

  /** Sends a TCP packet */
  static bool send(IpAddress dest,
                   uint16_t srcPort,
                   uint16_t destPort,
                   uint32_t seqNumber,
                   uint32_t ackNumber,
                   uint8_t flags,
                   uint16_t window,
                   size_t nBytes,
                   uintptr_t payload);

  /** Calculates a TCP checksum */
  uint16_t tcpChecksum(IpAddress srcip, IpAddress destip, tcpHeader* data, uint16_t len);

  /// \todo Work on my hex to be able to do this in my head rather than use decimal numbers
  enum TcpFlag
  {
    FIN = 1,
    SYN = 2,
    RST = 4,
    PSH = 8,
    ACK = 16,
    URG = 32,
    ECE = 64,
    CWR = 128
  };

  enum TcpOption
  {
    OPT_END = 0,
    OPT_NOP,
    OPT_MSS,
    OPT_WSS,
    OPT_SACK,
    OPT_TMSTAMP
  };

  enum TcpState
  {
    LISTEN          = 0,
    SYN_SENT        = 1,
    SYN_RECEIVED    = 2,
    ESTABLISHED     = 3,
    FIN_WAIT_1      = 4,
    FIN_WAIT_2      = 5,
    CLOSE_WAIT      = 6,
    CLOSING         = 7,
    LAST_ACK        = 8,
    TIME_WAIT       = 9,
    CLOSED          = 10,
    UNKNOWN         = 11
  };

  // provides a string representation of a given state
  static const char* stateString(TcpState state)
  {
    switch(state)
    {
      case LISTEN:
        return "LISTEN";
      case SYN_SENT:
        return "SYN-SENT";
      case SYN_RECEIVED:
        return "SYN-RECEIVED";
      case ESTABLISHED:
        return "ESTABLISHED";
      case FIN_WAIT_1:
        return "FIN-WAIT-1";
      case FIN_WAIT_2:
        return "FIN-WAIT-2";
      case CLOSE_WAIT:
        return "CLOSE-WAIT";
      case CLOSING:
        return "CLOSING";
      case LAST_ACK:
        return "LAST-ACK";
      case TIME_WAIT:
        return "TIME-WAIT";
      case CLOSED:
        return "CLOSED";
      default:
        return "UNKNOWN";
    }
  }
};

#endif /* _NetworkStack_TCP_h_ */
