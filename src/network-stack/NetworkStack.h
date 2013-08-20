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
#ifndef _NetworkStack_NetworkStack_h_
#define _NetworkStack_NetworkStack_h_ 1

#include "util/String.h"
#include "util/Vector.h"
#include "mach/Network.h"
#include "util/RequestQueue.h"
#include "util/MemoryPool.h"

/**
 * The Pedigree network stack
 * This function is the base for receiving packets, and provides functionality
 * for keeping track of network devices in the system.
 */
class NetworkStack : public RequestQueue
{
public:
  NetworkStack();
  virtual ~NetworkStack();

  /** For access to the stack without declaring an instance of it */
  static NetworkStack& instance() {
    return stack;
  }

  /** Called when a packet arrives */
  void receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset);

  /** Registers a given network device with the stack */
  void registerDevice(Network *pDevice);

  /** Returns the n'th registered network device */
  Network *getDevice(size_t n);

  /** Returns the number of devices registered with the stack */
  size_t getNumDevices();

  /** Unregisters a given network device from the stack */
  void deRegisterDevice(Network *pDevice);

  /** Sets the loopback device for the stack */
  void setLoopback(Network *pCard) {
    m_pLoopback = pCard;
  }

  /** Gets the loopback device for the stack */
  inline Network *getLoopback() {
    return m_pLoopback;
  }

  /** Grabs the memory pool for networking use */
  inline MemoryPool &getMemPool() {
    return m_MemPool;
  }

private:

  static NetworkStack stack;

  static void packetThread(void *p);

  struct Packet {
    uintptr_t buffer;
    size_t packetLength;
    Network *pCard;
    uint32_t offset;
  };

  virtual uint64_t executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                                  uint64_t p6, uint64_t p7, uint64_t p8);

  /** Loopback device */
  Network *m_pLoopback;

  /** Network devices registered with the stack. */
  Vector<Network*> m_Children;

  /** Networking memory pool */
  MemoryPool m_MemPool;
};

#endif /* _NetworkStack_NetworkStack_h_ */
