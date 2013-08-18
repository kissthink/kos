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
#include "mach/platform.h"
#include "util/RequestQueue.h"
#include "util/Output.h"
#include "kern/Thread.h"

RequestQueue::RequestQueue() :
  m_Stop(false), m_RequestQueueLock()
  , m_RequestQueueSize(), m_pThread(0)
{
    for (size_t i = 0; i < REQUEST_QUEUE_NUM_PRIORITIES; i++)
        m_pRequestQueue[i] = 0;
}

RequestQueue::~RequestQueue()
{
}

void RequestQueue::initialise()
{
  // Start the worker thread.
  if(m_pThread)
  {
    ABORT1("RequestQueue initialized multiple times");
  }
  m_pThread = Thread::create(kernelSpace);
  kernelScheduler.run( *m_pThread, trampoline, this );
}

void RequestQueue::destroy()
{
  // Cause the worker thread to stop.
  m_Stop = true;
  // Post to the queue length semaphore to ensure the worker thread wakes up.
  m_RequestQueueSize.V();
}

uint64_t RequestQueue::addRequest(size_t priority, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
                                  uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8)
{
  // Create a new request object.
  Request *pReq = new Request();
  pReq->p1 = p1; pReq->p2 = p2; pReq->p3 = p3; pReq->p4 = p4; pReq->p5 = p5; pReq->p6 = p6; pReq->p7 = p7; pReq->p8 = p8;
  pReq->isAsync = false;
  pReq->next = 0;
  pReq->bReject = false;
  pReq->pThread = Processor::getCurrThread();
//  pReq->pThread->addRequest(pReq);

  // Add to the request queue.
  m_RequestQueueLock.acquire();

  if (m_pRequestQueue[priority] == 0) {
    m_pRequestQueue[priority] = pReq;
  } else {
    Request *p = m_pRequestQueue[priority];
    while (p->next != 0)
      p = p->next;
    p->next = pReq;
  }

  // Increment the number of items on the request queue.
  m_RequestQueueSize.V();

  m_RequestQueueLock.release(); // finished adding a new request

  if(pReq->bReject) {
    // Hmm, in the time the RequestQueueLock was being acquired, we got
    // pre-empted, and then an unexpected exit event happened. The request
    // is to be rejected, so don't acquire the mutex at all.
    delete pReq;
    return 0;
  }

  // Wait for the request to be satisfied. This should sleep the thread.
  pReq->sem.P();

  if(pReq->bReject/* || pThread->wasInterrupted()*/) {
    // The request was interrupted somehow. We cannot assume that pReq's
    // contents are valid, so just return zero. The caller may have to redo
    // their request.
    // By releasing here, the worker thread can detect that the request was
    // interrupted and clean up by itself.
    DBG::outln(DBG::Basic, "RequestQueue::addRequest - interrupted");
    if(pReq->bReject)
        delete pReq; // Safe to delete, unexpected exit condition
    pReq->sem.V();
    return 0;
  }

  // Grab the result.
  uintptr_t ret = pReq->ret;
  delete pReq;

  return ret;
}

uint64_t RequestQueue::addAsyncRequest(size_t priority, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
                                       uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8)
{
  // Create a new request object.
  Request *pReq = new Request();
  pReq->p1 = p1; pReq->p2 = p2; pReq->p3 = p3; pReq->p4 = p4; pReq->p5 = p5; pReq->p6 = p6; pReq->p7 = p7; pReq->p8 = p8;
  pReq->isAsync = true;
  pReq->next = 0;
  pReq->bReject = false;
  pReq->pThread = Processor::getCurrThread();
//  pReq->pThread->addRequest(pReq);

  // Add to the request queue.
  m_RequestQueueLock.acquire();

  if (m_pRequestQueue[priority] == 0) {
    m_pRequestQueue[priority] = pReq;
  } else {
    Request *p = m_pRequestQueue[priority];
    while (p->next != 0) {
      if(p == pReq) { // prevent multiple queueing
        return 0;
      }
      p = p->next;
    }
    if(p != pReq) {
      p->next = pReq;
    } else {
      return 0;
    }
  }

  // Increment the number of items on the request queue.
  m_RequestQueueSize.V();

  // If the queue is too long, make this block.
  if (m_RequestQueueSize.getValue() > REQUEST_QUEUE_MAX_QUEUE_SZ) {
    pReq->isAsync = false;

    m_RequestQueueLock.release();

    // Wait for the request to be satisfied. This should sleep the thread.
    pReq->sem.P();

    if(pReq->bReject/* || pThread->wasInterrupted()*/) {
      // The request was interrupted somehow. We cannot assume that pReq's
      // contents are valid, so just return zero. The caller may have to redo
      // their request.
      // By releasing here, the worker thread can detect that the request was
      // interrupted and clean up by itself.
      DBG::outln(DBG::Basic, "RequestQueue::addRequest interrupted");
      pReq->sem.V();
      return 0;
    }

    // Delete the request structure.
    delete pReq;
  } else {
    m_RequestQueueLock.release();
  }

  return 0;
}

static void trampoline(ptr_t p) {
  RequestQueue *pRQ = reinterpret_cast<RequestQueue*>(p);
  pRQ->work();
}

int RequestQueue::work() {
  for(;;) {
    // Sleep on the queue length semaphore - wake when there's something to do.
    m_RequestQueueSize.P();

    // Check why we were woken - is m_Stop set? If so, quit.
    if (m_Stop)
      return 0;

    // Get the first request from the queue.
    m_RequestQueueLock.acquire();

    // Get the most important queue with data in.
    /// \todo Stop possible starvation here.
    size_t priority = 0;
    for (priority = 0; priority < REQUEST_QUEUE_NUM_PRIORITIES-1; priority++)
        if (m_pRequestQueue[priority])
            break;

    Request *pReq = m_pRequestQueue[priority];
    // Quick sanity check:
    if (pReq == 0)
      continue;

    m_pRequestQueue[priority] = pReq->next;

    m_RequestQueueLock.release();

    // Verify that it's still valid to run the request
    if(pReq->bReject) {
        DBG::outln(DBG::Basic, "RequestQueue request rejected");
        if(pReq->isAsync)
            delete pReq;
        continue;
    }

    // Perform the request.
    pReq->ret = executeRequest(pReq->p1, pReq->p2, pReq->p3, pReq->p4, pReq->p5, pReq->p6, pReq->p7, pReq->p8);
    if(pReq->sem.tryP()) {
      // Something's gone wrong - the calling thread has released the Mutex. Destroy the request
      // and grab the next request from the queue. The calling thread has long since stopped
      // caring about whether we're done or not.
      DBG::outln(DBG::Basic, "RequestQueue::work caller interrupted");
      continue;
    }

    bool bAsync = pReq->isAsync;

    // Request finished - post the request's mutex to wake the calling thread.
    pReq->sem.V();

    // If the request was asynchronous, destroy the request structure.
    if (bAsync) {
      delete pReq;
    }
  }
  return 0;
}
