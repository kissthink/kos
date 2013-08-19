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
#ifndef _NetworkStack_DNS_h_
#define _NetworkStack_DNS_h_ 1

#include "util/String.h"
#include "util/Vector.h"
#include "util/Tree.h"
#include "ipc/BlockingSync.h"
#include "mach/Network.h"
#include "mach/Machine.h"

#include "network-stack/NetworkStack.h"
#include "network-stack/Ethernet.h"
#include "network-stack/UdpManager.h"

/** These are bit masks for the opAndParam field of the header */
#define DNS_QUESREQ    0x8000 // query = 0, request = 1
#define DNS_OPCODE     0x7800 // 0 = standard, 1 = inverse
#define DNS_AUTHANS    0x400 // 1 if authoritative answer
#define DNS_TRUNC      0x200 // 1 if the message is truncated
#define DNS_RECURSION  0x100 // 1 if recursion is wanted
#define DNS_RECURAVAIL 0x80 // 1 if recursion is available
#define DNS_RSVD       0x70
#define DNS_RESPONSE   0xF // response code - 0 means no errors

/** Query types */
#define DNSQUERY_HOSTADDR     1

/**
 * The KOS network stack - DNS implementation
 */
class Dns {
  Dns() = delete;
  Dns(const Dns& ent) = delete;
  Dns& operator=(const Dns&) = delete;

public:
  /** Information about a host (based on a specific hostname) */
  struct HostInfo
  {
      /// Hostname for this host, based on our request (probably a CNAME)
      String hostname;

      /// Aliases for this host (A records)
      List<String*> aliases;

      /// IP addresses for this host
      List<IpAddress*> addresses;
  };
  
  /** Main daemon thread */
  static void mainThread();

  /** Initialises the Endpoint and begins running the worker thread */
  static void initialise();
  
  /** Requests a lookup for a hostname */
  static int hostToIp(String hostname, HostInfo& ret, Network* pCard = 0);
  
  /** Operator = is invalid */
  Dns& operator = (const Dns& ent)
  {
    DBG::outln(DBG::Net, "Dns::operator = called!");
    return *this;
  }

private:

  static Dns dnsInstance;
  static uint16_t m_NextId;

  struct DnsHeader
  {
    uint16_t  id;
    uint16_t  opAndParam;
    uint16_t  qCount; // question entry count
    uint16_t  aCount; // answer entry count
    uint16_t  nCount; // nameserver count
    uint16_t  dCount; // additional data count
  } __attribute__ ((packed));
  
  struct QuestionSecNameSuffix
  {
    uint16_t  type;
    uint16_t  cls;
  } __attribute__ ((packed));
  
  struct DnsAnswer
  {
    uint16_t  name;
    uint16_t  type;
    uint16_t  cls;
    uint32_t  ttl;
    uint16_t  length;
  } __attribute__ ((packed));
  
  /** An entry in the DNS cache */
  class DnsEntry
  {
    public:
      DnsEntry() : ip(0), numIps(0), hostname()
      {};
      DnsEntry(const DnsEntry& ent) : ip(ent.ip), numIps(ent.numIps), hostname(ent.hostname)
      {};
      
      /// Multiple IP addresses are possible
      IpAddress* ip;
      size_t numIps;
      
      /// Hostname
      String hostname;
      
      DnsEntry& operator = (const DnsEntry& ent)
      {
        ip = ent.ip;
        numIps = ent.numIps;
        hostname = ent.hostname;
        return *this;
      }
  };
  
  /// a DNS request we've sent
  class DnsRequest
  {
    public:
      DnsRequest() :
        hostname(), aliases(), addresses(), id(0), waitSem(0),
        success(false), callerLeft(false)
      {};
      DnsRequest(const DnsRequest& ent) :
        hostname(ent.hostname), aliases(ent.aliases), addresses(ent.addresses), id(ent.id),
        waitSem(0), success(ent.success), callerLeft(ent.callerLeft)
      {};
      
      /// Hostname for this host, based on our request (probably a CNAME)
      String hostname;

      /// Aliases for this host (A records)
      List<String*> aliases;

      /// IP addresses for this host
      List<IpAddress*> addresses;

      /// DNS request ID
      uint16_t id;
      
      /// Semaphore used to wake up the caller thread when this request completes
      Semaphore waitSem;
      
      /// Whether or not the request succeeded (possibly redundant now)
      bool success;

      // Whether or not the caller left. If true, the request handler needs to
      // free the request rather than waking up the calling thread.
      bool callerLeft;
      
      DnsRequest& operator = (const DnsRequest& ent)
      {
        hostname = ent.hostname;
        aliases = ent.aliases;
        addresses = ent.addresses;
        id = ent.id;
        success = ent.success;
        callerLeft = ent.callerLeft;
        return *this;
      }
      
    private:
  };
  
  /// DNS cache (not a Tree because we can't look up an IpAddress object)
  static List<DnsEntry*> m_DnsCache;
  
  /// DNS request list
  static Vector<DnsRequest*> m_DnsRequests;
  
  /// DNS communication endpoint
  static ConnectionlessEndpoint* m_Endpoint;
};

#endif /* _NetworkStack_DNS_h_ */
