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
#ifndef _NetworkStack_UDPManager_h_
#define _NetworkStack_UDPManager_h_ 1

#include "util/String.h"
#include "util/Tree.h"
#include "util/List.h"
#include "ipc/BlockingSync.h"
#include "mach/Network.h"

#include "network-stack/Manager.h"
#include "network-stack/ConnectionlessEndpoint.h"
#include "network-stack/Endpoint.h"
#include "network-stack/Udp.h"

/**
 * The Pedigree network stack - UDP Endpoint
 * \todo This needs to keep track of a LOCAL IP as well
 *        so we can actually bind properly if there's multiple
 *        cards with different configurations.
 */
class UdpEndpoint : public ConnectionlessEndpoint
{
  public:

    /** Constructors and destructors */
    UdpEndpoint() :
      ConnectionlessEndpoint(), m_DataQueue(), m_DataQueueSize(0),
      m_bAcceptAll(false), m_bCanSend(true), m_bCanRecv(true)
    {};
    UdpEndpoint(uint16_t local, uint16_t remote) :
      ConnectionlessEndpoint(local, remote), m_DataQueue(),
      m_DataQueueSize(0), m_bAcceptAll(false), m_bCanSend(true),
      m_bCanRecv(true)
    {};
    UdpEndpoint(IpAddress remoteIp, uint16_t local = 0, uint16_t remote = 0) :
      ConnectionlessEndpoint(remoteIp, local, remote), m_DataQueue(),
      m_DataQueueSize(0), m_bAcceptAll(false), m_bCanSend(true),
      m_bCanRecv(true)
    {};

    virtual ~UdpEndpoint() {};

    /** Application interface */
    virtual int state() {return 0xff;} // 0xff signifies UDP
    virtual int send(size_t nBytes, uintptr_t buffer, RemoteEndpoint remoteHost, bool broadcast, Network *pCard = 0);
    virtual int recv(uintptr_t buffer, size_t maxSize, bool bBlock, RemoteEndpoint* remoteHost, int nTimeout = 30);
    virtual bool dataReady(bool block = false, uint32_t tmout = 30);
    virtual inline bool acceptAnyAddress() { return m_bAcceptAll; };
    virtual inline void acceptAnyAddress(bool accept) { m_bAcceptAll = accept; };

    /** UdpManager functionality - called to deposit data into our local buffer */
    virtual size_t depositPayload(size_t nBytes, uintptr_t payload, RemoteEndpoint remoteHost);

    /** Shutdown on a UDP endpoint merely disables via software the ability to send/recv */
    virtual bool shutdown(ShutdownType what)
    {
        if(what == ShutSending)
            m_bCanSend = false;
        else if(what == ShutReceiving)
            m_bCanRecv = false;
        else if(what == ShutBoth)
            m_bCanRecv = m_bCanSend = false;
        return true;
    }

  private:

    struct DataBlock
    {
      DataBlock() :
        magic(0xdeadbeef), size(0), offset(0), ptr(0), remoteHost()
      {};

      uint32_t magic; // 0xdeadbeef

      size_t size;
      size_t offset; // if we only do a partial read, this is filled
      uintptr_t ptr;

      RemoteEndpoint remoteHost; // who sent it to us - needed for port info!
    };

    /** Incoming data queue */
    List<DataBlock*> m_DataQueue;

    /** Data queue size */
    Semaphore m_DataQueueSize;

    /** Accept any address? */
    bool m_bAcceptAll;

    /** Can we send? */
    bool m_bCanSend;

    /** Can we receive? */
    bool m_bCanRecv;
};

/**
 * The Pedigree network stack - UDP Protocol Manager
 */
class UdpManager : public ProtocolManager
{
public:
  UdpManager() :
    m_Endpoints()
  {};
  virtual ~UdpManager()
  {};

  /** For access to the manager without declaring an instance of it */
  static UdpManager& instance()
  {
    return manager;
  }

  /** Gets a new Endpoint */
  Endpoint* getEndpoint(IpAddress remoteHost, uint16_t localPort, uint16_t remotePort);

  /** Returns an Endpoint */
  void returnEndpoint(Endpoint* e);

  /** A new packet has arrived! */
  void receive(IpAddress from, IpAddress to, uint16_t sourcePort, uint16_t destPort, uintptr_t payload, size_t payloadSize, Network* pCard);

private:

  static UdpManager manager;

  /** Currently known endpoints (all actually UdpEndpoints). */
  Tree<size_t, Endpoint*> m_Endpoints;
};

#endif /* _NetworkStack_UDPManager_h_ */
