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

#include "network-stack/Icmp.h"
#include "util/Module.h"
#include "kern/Debug.h"
#include "util/Output.h"
#include "util/endian.h"
#include "network-stack/Ethernet.h"
#include "network-stack/Ipv4.h"

#include "network-stack/Filter.h"

// #define DEBUG_ICMP

Icmp Icmp::icmpInstance;

Icmp::Icmp()
{
}

Icmp::~Icmp()
{
}

void Icmp::send(IpAddress dest, uint8_t type, uint8_t code, uint16_t id, uint16_t seq, size_t nBytes, uintptr_t payload, Network *pCard)
{
  StationInfo me = pCard->getStationInfo();
  if(me.ipv4 == 0)
    return; // We're not configured yet.
  
  uintptr_t packet = NetworkStack::instance().getMemPool().allocate();

  icmpHeader* header = reinterpret_cast<icmpHeader*>(packet);
  header->type = type;
  header->code = code;
  header->id = HOST_TO_BIG16(id);
  header->seq = HOST_TO_BIG16(seq);

  if(nBytes)
    memcpy(reinterpret_cast<void*>(packet + sizeof(icmpHeader)), reinterpret_cast<void*>(payload), nBytes);

  header->checksum = 0;
  header->checksum = Network::calculateChecksum(packet, nBytes + sizeof(icmpHeader));

  /// \note Assume IPv4, as ICMPv6 exists for IPv6.
  Ipv4::instance().send(dest, Network::convertToIpv4(0, 0, 0, 0), IP_ICMP, nBytes + sizeof(icmpHeader), packet, pCard);
  
  NetworkStack::instance().getMemPool().free(packet);
}

void Icmp::receive(IpAddress from, IpAddress to, uintptr_t packet, size_t nBytes, IpBase *pIp, Network* pCard)
{
  if(!packet || !nBytes)
      return;

  // Check for filtering
  if(!NetworkFilter::instance().filter(3, packet, nBytes))
  {
    pCard->droppedPacket();
    return;
  }
  
  // Grab the header
  icmpHeader* header = reinterpret_cast<icmpHeader*>(packet);

  DBG::outln(DBG::Net, "ICMP type=", FmtHex(header->type), ", code=", FmtHex(header->code), ", checksum=", FmtHex(header->checksum));
  DBG::outln(DBG::Net, "ICMP id=", FmtHex(header->id), ", seq=", FmtHex(header->seq));

  // check the checksum
  uint16_t checksum = header->checksum;
  header->checksum = 0;
  uint16_t calcChecksum = Network::calculateChecksum(packet, nBytes);
  header->checksum = checksum;

  DBG::outln(DBG::Net, "ICMP calculated checksum is ", FmtHex(calcChecksum), ", packet checksum = ", FmtHex(checksum));
  
  if(checksum == calcChecksum)
  {
    // what's come in?
    switch(header->type)
    {
      case ICMP_ECHO_REQUEST:
        {

        DBG::outln(DBG::Net, "ICMP: Echo request from ", from.toString(), ".");

        // send the reply
        send(
          from,
          ICMP_ECHO_REPLY,
          header->code,
          BIG_TO_HOST16(header->id),
          BIG_TO_HOST16(header->seq),
          nBytes - sizeof(icmpHeader),
          packet + sizeof(icmpHeader),
          pCard
        );

        }
        break;

      default:

        // Now that things can be moved out to user applications thanks to SOCK_RAW,
        // the kernel doesn't need to implement too much of the ICMP suite.

        DBG::outln(DBG::Net, "ICMP: Unhandled packet - type is ", header->type, ".");
        break;
    }
  }
  else
  {
    DBG::outln(DBG::Warning, "ICMP: invalid checksum on incoming packet.");
    pCard->badPacket();
  }
}
