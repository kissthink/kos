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
#ifndef _NetworkStack_RawManager_h_
#define _NetworkStack_RawManager_h_ 1

#include "util/String.h"
#include "util/Tree.h"
#include "util/List.h"
#include "ipc/BlockingSync.h"
#include "mach/Network.h"

#include "network-stack/Manager.h"

#include "network-stack/ConnectionlessEndpoint.h"
#include "network-stack/Endpoint.h"
#include "network-stack/Ethernet.h"

/**
 * The Pedigree network stack - RAW Endpoint
 * \todo This is really messy - needs a rewrite at some point!
 */
class RawEndpoint : public ConnectionlessEndpoint
{
public:

  /** What type is this Raw Endpoint? */
  enum Type {
    RAW_WIRE = 0, // wire-level
    RAW_ICMP, // IP levels
    RAW_UDP,
    RAW_TCP
  };

  /** Constructors and destructors */
  RawEndpoint() :
    ConnectionlessEndpoint(), m_DataQueue(), m_DataQueueSize(0),
    m_bAcceptAll(false), m_Type(RAW_WIRE)
  {};

  /** These shouldn't be used - totally pointless */
  RawEndpoint(uint16_t local, uint16_t remote) :
    ConnectionlessEndpoint(local, remote), m_DataQueue(),
    m_DataQueueSize(0), m_bAcceptAll(false), m_Type(RAW_WIRE)
  {};
  RawEndpoint(IpAddress remoteIp, uint16_t local = 0, uint16_t remote = 0) :
    ConnectionlessEndpoint(remoteIp, local, remote), m_DataQueue(),
    m_DataQueueSize(0), m_bAcceptAll(false), m_Type(RAW_WIRE)
  {};
  RawEndpoint(Type type) :
    ConnectionlessEndpoint(0, 0), m_DataQueue(), m_DataQueueSize(0),
    m_bAcceptAll(false), m_Type(type)
  {};
  virtual ~RawEndpoint();

  /** Injects the given buffer into the network stack */
  virtual int send(size_t nBytes, uintptr_t buffer, Endpoint::RemoteEndpoint remoteHost, bool broadcast, Network *pCard = 0);

  /** Reads from the front of the packet queue. Will return truncated packets if maxSize < packet size. */
  virtual int recv(uintptr_t buffer, size_t maxSize, bool bBlock, Endpoint::RemoteEndpoint* remoteHost, int nTimeout = 30);

  /** Are there packets to read? */
  virtual bool dataReady(bool block = false, uint32_t tmout = 30);

  /** Deposits a packet into this endpoint */
  virtual size_t depositPacket(size_t nBytes, uintptr_t payload, Endpoint::RemoteEndpoint* remoteHost);

  /** What type is this endpoint? */
  inline Type getRawType() {
    return m_Type;
  }

  /** Shutdown on a RAW endpoint does nothing */
  virtual bool shutdown(ShutdownType what) {
    return true;
  }

private:

  struct DataBlock {
    DataBlock() :
      size(0), ptr(0), remoteHost()
    {};

    size_t size;
    uintptr_t ptr;
    Endpoint::RemoteEndpoint remoteHost;
  };

  /** Incoming data queue */
  List<DataBlock*> m_DataQueue;

  /** Data queue size */
  Semaphore m_DataQueueSize;

  /** Accept any address? */
  bool m_bAcceptAll;

  /** Our type */
  Type m_Type;
};

/**
 * The Pedigree network stack - RAW Protocol Manager
 */
class RawManager : public ProtocolManager
{
public:
  RawManager() :
    m_Endpoints()
  {};
  virtual ~RawManager()
  {};

  /** For access to the manager without declaring an instance of it */
  static RawManager& instance() {
    return manager;
  }

  /** Gets a new Endpoint */
  Endpoint* getEndpoint(int proto); //IpAddress remoteHost, uint16_t localPort, uint16_t remotePort);

  /** Returns an Endpoint */
  void returnEndpoint(Endpoint* e);

  /** A new packet has arrived! */
  void receive(uintptr_t payload, size_t payloadSize, Endpoint::RemoteEndpoint* remoteHost, int proto, Network* pCard);

private:

  static RawManager manager;

  /** Currently known endpoints (all actually RawEndpoints - each one is passed incoming packets). */
  List<Endpoint*> m_Endpoints;
};

#endif /* _NetworkStack_RawManager_h_ */
