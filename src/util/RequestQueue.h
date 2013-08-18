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

#ifndef _RequestQueue_h_
#define _RequestQueue_h_ 1

#include "ipc/BlockingSync.h"
#include "ipc/SpinLock.h"
#include "mach/platform.h"

class Thread;
static void trampoline(ptr_t);

#define REQUEST_QUEUE_NUM_PRIORITIES 4
#define REQUEST_QUEUE_MAX_QUEUE_SZ   32

/** Implements a request queue, with one worker thread performing
 * all requests. All requests appear synchronous to the calling thread -
 * calling threads are blocked on mutexes (so they can be put to sleep) until
 * their request is complete. */
class RequestQueue
{
    friend class Thread;
    friend void trampoline(ptr_t);
public:
    /** Creates a new RequestQueue. */
    RequestQueue();
    virtual ~RequestQueue();

    /** Initialises the queue, spawning the worker thread. */
    virtual void initialise();

    /** Destroys the queue, killing the worker thread (safely) */
    virtual void destroy();

    /** Adds a request to the queue. Blocks until it finishes and returns the result.
        \param priority The priority to attach to this request. Lower number is higher priority. */
    uint64_t addRequest(size_t priority, uint64_t p1=0, uint64_t p2=0, uint64_t p3=0, uint64_t p4=0, uint64_t p5=0,
                        uint64_t p6=0, uint64_t p7=0, uint64_t p8=0);

    /** Adds an asynchronous request to the queue. Will not block. */
    uint64_t addAsyncRequest(size_t priority, uint64_t p1=0, uint64_t p2=0, uint64_t p3=0, uint64_t p4=0, uint64_t p5=0,
                             uint64_t p6=0, uint64_t p7=0, uint64_t p8=0);

protected:
    /** Callback - classes are expected to inherit and override this function. It's called when a
        request needs to be executed (by the worker thread). */
    virtual uint64_t executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                                    uint64_t p6, uint64_t p7, uint64_t p8) = 0;

    RequestQueue(const RequestQueue&);
    void operator =(const RequestQueue&);

    /** Request structure */
    class Request
    {
    public:
        Request() : p1(0),p2(0),p3(0),p4(0),p5(0),p6(0),p7(0),p8(0),ret(0),
                    sem(),pThread(0),
                    isAsync(false),bReject(false),next(0) {}
        ~Request() {}
        uint64_t p1,p2,p3,p4,p5,p6,p7,p8;
        uint64_t ret;
        Semaphore sem;
        Thread *pThread;
        bool isAsync;
        bool bReject;
        Request *next;
    private:
        Request(const Request&) = delete;
        Request& operator =(const Request&) = delete;
    };

    /** Thread worker function */
    int work();

    /** The request queue */
    Request *m_pRequestQueue[REQUEST_QUEUE_NUM_PRIORITIES];

    /** True if the worker thread should cleanup and stop. */
    volatile bool m_Stop;

    /** Mutex to be held when the request queue is being changed. */
    SpinLock m_RequestQueueLock;

    /** The semaphore giving the number of items in the queue. */
    Semaphore m_RequestQueueSize;
    
    Thread *m_pThread;
};

#endif /* _RequestQueue_h_ */
