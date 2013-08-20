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
#ifndef _NetworkStack_Arp_h_
#define _NetworkStack_Arp_h_ 1

#include "util/String.h"
#include "util/Vector.h"
#include "util/Tree.h"
#include "ipc/BlockingSync.h"
#include "mach/Network.h"
#include "mach/Machine.h"
#include "util/RequestQueue.h"

#include "network-stack/NetworkStack.h"
#include "network-stack/Ethernet.h"

#define ARP_OP_REQUEST  0x0001
#define ARP_OP_REPLY    0x0002

/**
 * The Pedigree network stack - ARP layer
 */
class Arp : public RequestQueue
{
public:
  Arp();
  virtual ~Arp();

  /** For access to the stack without declaring an instance of it */
  static Arp& instance() {
    return arpInstance;
  }

  /** Packet arrival callback */
  void receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset);

  /** Sends an ARP request */
  void send(IpAddress req, Network* pCard = 0);

  /** Gets an entry from the ARP cache, and optionally resolves it if needed. */
  bool getFromCache(IpAddress ip, bool resolve, MacAddress* ent, Network* pCard);

  /** Direct cache manipulation */
  bool isInCache(IpAddress ip);
  void insertToCache(IpAddress ip, MacAddress mac);
  void removeFromCache(IpAddress ip);

private:

  /** ARP request doer */
  virtual uint64_t executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                                  uint64_t p6, uint64_t p7, uint64_t p8);

  static Arp arpInstance;

  struct arpHeader {
    uint16_t  hwType;
    uint16_t  protocol;
    uint8_t   hwSize;
    uint8_t   protocolSize;
    uint16_t  opcode;
    uint8_t   hwSrc[6];
    uint32_t  ipSrc;
    uint8_t   hwDest[6];
    uint32_t  ipDest;
  } __attribute__ ((packed));

  // an entry in the arp cache
  /// \todo Will need to store *time* and *type* - time for removing from cache
  ///       and type for static entries
  struct arpEntry {
    arpEntry() :
      valid(false), ip(), mac()
    {};

    bool valid;
    IpAddress ip;
    MacAddress mac;
  };

  // an ARP request we've sent
  class ArpRequest // : public TimerHandler
  {
  public:
    ArpRequest() :
      destIp(), mac(), waitSem(0), success(false)
    {};

    IpAddress destIp;
    MacAddress mac;
    Semaphore waitSem;

    bool success;
  };

  // ARP Cache
  Tree<size_t, arpEntry*> m_ArpCache;

  // ARP request list
  Vector<ArpRequest*> m_ArpRequests;

};

#endif /* _NetworkStack_Arp_h_ */
