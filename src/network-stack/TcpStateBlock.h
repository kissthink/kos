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
#ifndef _NetworkStack_TCPStateBlock_h_
#define _NetworkStack_TCPStateBlock_h_ 1

#include "util/basics.h"
#include "ipc/BlockingSync.h"
#include "mach/Network.h"
#include "kern/TimerEvent.h"
#include "kern/DynamicTimer.h"

#include "network-stack/NetworkStack.h"
#include "network-stack/Endpoint.h"
#include "network-stack/TcpMisc.h"
#include "network-stack/Tcp.h"

/// \todo Eventify.

/// This is passed a given StateBlock and its sole purpose is to remove it
/// from the system. It's called as a thread when the TIME_WAIT timeout expires
/// to enable the block to be freed without requiring intervention.
void stateBlockFree(void* p);

// TCP is based on connections, so we need to keep track of them
// before we even think about depositing into Endpoints. These state blocks
// keep track of important information relating to the connection state.
class StateBlock : public TimerEvent
{
private:

  struct Segment {
    uint32_t  seg_seq; // Segment sequence number
    uint32_t  seg_ack; // Ack number
    uint32_t  seg_len; // Segment length
    uint32_t  seg_wnd; // Segment window
    uint32_t  seg_up; // Urgent pointer
    uint8_t   flags;

    uintptr_t payload;
    size_t    nBytes;
  };

public:
  StateBlock() : TimerEvent(10000, true),
    currentState(Tcp::CLOSED), localPort(0), remoteHost(),
    iss(0), snd_nxt(0), snd_una(0), snd_wnd(0), snd_up(0), snd_wl1(0), snd_wl2(0),
    rcv_nxt(0), rcv_wnd(0), rcv_up(0), irs(0),
    seg_seq(0), seg_ack(0), seg_len(0), seg_wnd(0), seg_up(0), seg_prc(0),
    fin_ack(false), fin_seq(0),
    numEndpointPackets(0), /// \todo Remove, obsolete
    waitState(0), endpoint(0), connId(0),
    retransmitQueue(), nRemovedFromRetransmit(0),
    waitingForTimeout(false), didTimeout(false), timeoutWait(0), useWaitSem(true),
    m_Nanoseconds(0), m_Seconds(0), m_Timeout(10) {
    DynamicTimer::registerEvent(this);
  };
  ~StateBlock() {
    DynamicTimer::unregisterEvent(this);
  };

  Tcp::TcpState currentState;

  uint16_t localPort;

  Endpoint::RemoteEndpoint remoteHost;

  // Send sequence variables
  uint32_t iss; // initial sender sequence number (CLIENT)
  uint32_t snd_nxt; // next send sequence number
  uint32_t snd_una; // send unack
  uint32_t snd_wnd; // send window ----> How much they can receive max
  uint32_t snd_up; // urgent pointer?
  uint32_t snd_wl1; // segment sequence number for last WND update
  uint32_t snd_wl2; // segment ack number for last WND update

  // Receive sequence variables
  uint32_t rcv_nxt; // receive next - what we're expecting perhaps?
  uint32_t rcv_wnd; // receive window ----> How much we want to receive methinks...
  uint32_t rcv_up; // receive urgent pointer
  uint32_t irs; // initial receiver sequence number (SERVER)

  // Segment variables
  uint32_t seg_seq; // segment sequence number
  uint32_t seg_ack; // ack number
  uint32_t seg_len; // segment length
  uint32_t seg_wnd; // segment window
  uint32_t seg_up; // urgent pointer
  uint32_t seg_prc; // precedence

  // FIN information
  bool     fin_ack; // is ACK already set (for use with FIN bit checks)
  uint32_t fin_seq; // last FIN we sent had this sequence number

  // Number of packets we've deposited into our Endpoint
  // (decremented when a packet is picked up by the receiver)
  uint32_t numEndpointPackets;

  // Waiting for something?
  Semaphore waitState;

  // The endpoint applications use for this TCP connection
  TcpEndpoint* endpoint;

  // the id of this specific connection
  size_t connId;

  // Retransmission queue
  //TcpBuffer retransmitQueue;
  List<void*> retransmitQueue;

  // Number of bytes removed from the retransmit queue
  size_t nRemovedFromRetransmit;

  /// Handles a segment ack
  /// \note This will remove acked segments, however if there is only a partial ack on a segment
  ///       it will split it into two, remove the first, and leave a partial segment on the queue.
  ///       This behaviour does not affect anything internally as long as this function is always
  ///       used to acknowledge segments.
  void ackSegment() {
    // we assume the seg_* variables have been set by the caller (always done in TcpManager::receive)
    uint32_t segAck = seg_ack;
    while(retransmitQueue.count()) {
      // grab the first segment from the queue
      Segment* seg = reinterpret_cast<Segment*>(retransmitQueue.popFront());
      if((seg->seg_seq + seg->seg_len) <= segAck) {
        // this segment is acked, leave it off the queue and free the memory used
        if(seg->payload)
          delete [] (reinterpret_cast<uint8_t*>(seg->payload));

        delete seg;
        continue;
      } else {
        // check if the ack is within this segment
        if(segAck >= seg->seg_seq) {
          // it is, so we need to split the segment payload
          Segment* splitSeg = new Segment;
          *splitSeg = *seg;

          // how many bytes are acked?
          /// \bug This calculation *may* have an off-by-one error
          size_t nBytesAcked = segAck - seg->seg_seq;

          // update the sequence number
          splitSeg->seg_seq = seg->seg_seq + nBytesAcked;
          splitSeg->seg_len -= nBytesAcked;

          // and most importantly, recopy the payload
          if(seg->nBytes && seg->payload) {
            uint8_t* newPayload = new uint8_t[splitSeg->seg_len];
            memcpy(newPayload, reinterpret_cast<void*>(seg->payload), seg->nBytes);

            splitSeg->payload = reinterpret_cast<uintptr_t>(newPayload);
          }

          // push on the front, and don't continue (we know there's no potential for further ACKs)
          retransmitQueue.pushFront(reinterpret_cast<void*>(splitSeg));
          if(seg->payload)
            delete [] (reinterpret_cast<uint8_t*>(seg->payload));
          delete seg;
          return;
        }
      }
    }
  }

  /// Sends a segment over the network
  bool sendSegment(Segment* seg) {
    if(seg) {
      return Tcp::send(remoteHost.ip, localPort, remoteHost.remotePort, seg->seg_seq, rcv_nxt, seg->flags, seg->seg_wnd, seg->nBytes, seg->payload);
    }
    return false;
  }

  /// Sends a segment over the network
  bool sendSegment(uint8_t flags, size_t nBytes, uintptr_t payload, bool addToRetransmitQueue) {
    // split the passed buffer up into 1024 byte segments and send each
    size_t offset;
    for(offset = 0; offset < (nBytes == 0 ? 1 : nBytes); offset += 1024) {
      Segment* seg = new Segment;

      size_t segmentSize = 1024;
      if((offset + 1024) >= nBytes)
        segmentSize = nBytes - offset;

      seg_seq = snd_nxt;
      snd_nxt += segmentSize;

      seg->seg_seq = seg_seq;
      seg->seg_ack = rcv_nxt;
      seg->seg_len = segmentSize;
      seg->seg_wnd = snd_wnd;
      seg->seg_up = 0;
      seg->flags = flags;

      if(nBytes && payload) {
        uint8_t* newPayload = new uint8_t[segmentSize];
        memcpy(newPayload, reinterpret_cast<void*>(payload + offset), segmentSize);

        seg->payload = reinterpret_cast<uintptr_t>(newPayload);
      } else
        seg->payload = 0;
      seg->nBytes = seg->seg_len;

      sendSegment(seg);

      if(addToRetransmitQueue)
        retransmitQueue.pushBack(reinterpret_cast<void*>(seg));
      else
        delete seg;
    }

    return true;
  }

  // timer for all retransmissions (and state changes such as TIME_WAIT)
  bool run(mword curTick) {
    if (timedout()) {
      waitingForTimeout = false;
      didTimeout = true;
      if (useWaitSem) timeoutWait.V();
      // check to see if there's data on the retransmission queue to send
      if (retransmitQueue.count()) {
        DBG::outln(DBG::Net, "Remote TCP did not ack all the data!");
        // still more data unacked - grab the first segment and transmit it
        // note that we don't pop it off the queue permanently, as we are still
        // waiting for an ack for the segment
        Segment* seg = reinterpret_cast<Segment*>(retransmitQueue.popFront());
        sendSegment(seg);
        retransmitQueue.pushFront(reinterpret_cast<void*>(seg));

        // reset the timeout
        resetTimer();
      } else if (currentState == Tcp::TIME_WAIT) {
        // timer has fired, we need to close the connection
        DBG::outln(DBG::Net, "TIME_WAIT timeout complete");
        currentState = Tcp::CLOSED;

        // create the cleanup thread
        Thread* t = Thread::create(kernelSpace);
        kernelScheduler.run(*t, stateBlockFree, this);
      }
      return true;
    }
    return false; // did not timeout
  }

  // resets the timer (to restart a timeout)
  void resetTimer(uint32_t timeout = 10) {
    m_Seconds = m_Nanoseconds = 0;
    m_Timeout = timeout;
    didTimeout = false;
    reset(timeout);
  }

  // are we waiting on a timeout?
  bool waitingForTimeout;

  // did the action time out or not?
  /// \note This ensures that, if we end up releasing the timeout wait semaphore
  ///       via a non-timeout source (such as a data ack) we know where the release
  ///       actually came from.
  bool didTimeout;

  // timeout wait semaphore (in case)
  Semaphore timeoutWait;
  bool useWaitSem;

private:

  // number of nanoseconds & seconds for the timer
  uint64_t m_Nanoseconds;
  uint64_t m_Seconds;
  uint32_t m_Timeout;

  StateBlock(const StateBlock& s) : TimerEvent(10000, true),
    currentState(Tcp::CLOSED), localPort(0), remoteHost(),
    iss(0), snd_nxt(0), snd_una(0), snd_wnd(0), snd_up(0), snd_wl1(0), snd_wl2(0),
    rcv_nxt(0), rcv_wnd(0), rcv_up(0), irs(0),
    seg_seq(0), seg_ack(0), seg_len(0), seg_wnd(0), seg_up(0), seg_prc(0),
    fin_ack(false), fin_seq(0),
    numEndpointPackets(0), /// \todo Remove, obsolete
    waitState(0), endpoint(0), connId(0),
    retransmitQueue(), nRemovedFromRetransmit(0),
    waitingForTimeout(false), didTimeout(false), timeoutWait(0), useWaitSem(true),
    m_Nanoseconds(0), m_Seconds(0), m_Timeout(10000) /* about 10 sec */ {
    // same as TcpEndpoint - the copy constructor should not be called
    DBG::outln(DBG::Error, "Tcp: StateBlock copy constructor called");
  }
  StateBlock& operator = (const StateBlock& s) {
    // this isn't actually correct EITHER
    DBG::outln(DBG::Error, "Tcp: StateBlock copy constructor has been called.");
    return *this;
  }
};

#endif /* _NetworkStack_TCPStateBlock_h_ */
