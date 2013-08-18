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

#ifndef _LockedFile_h_
#define _LockedFile_h_ 1

#include "ipc/BlockingSync.h"
#include "mach/Processor.h"
#include "mach/platform.h"
#include "vfs/File.h"

/** LockedFile is a wrapper around a standard File with the ability to lock
 * access to it. Locked access is in the form of a Mutex that allows only
 * one thread exclusive access to the file.
 */
class LockedFile
{
public:

    /** Standard wrapper constructor */
    LockedFile(File *pFile) : m_File(pFile), m_bLocked(false), m_LockerThread(nullptr), m_Lock()
    {};

    /** Copy constructor */
    LockedFile(LockedFile &c) : m_File(0), m_bLocked(false), m_LockerThread(nullptr), m_Lock()
    {
        m_File = c.m_File;
        m_bLocked = c.m_bLocked;
        m_LockerThread = c.m_LockerThread;

        if(m_bLocked)
            m_Lock.acquire();
    }

    /** Operator = */
    /// \todo Write me!
    LockedFile & operator = (const LockedFile& c);

    /** Attempts to obtain the lock (exclusively) */
    bool lock(bool bBlock = false)
    {
        if(!bBlock)
        {
            if(!m_Lock.tryAcquire())
                return false;
        }
        else
            m_Lock.acquire();

        // Obtained the lock
        m_bLocked = true;
        m_LockerThread = Processor::getCurrThread();
        return true;
    }

    /** Releases the lock */
    void unlock()
    {
        if(m_bLocked)
        {
            m_bLocked = false;
            m_Lock.release();
        }
    }

    /** To enforce mandatory locking, use this function to obtain a File to
     * work with. If the file is locked and you don't own the lock, you'll
     * get a NULL File. Otherwise you'll get the wrapped File ready for I/O.
     */
    File *getFile()
    {
        // If we're locked, and we aren't the locking process, we can't access the file
        // Otherwise, the file is accessible
        if(m_bLocked == true && Processor::getCurrThread() != m_LockerThread)
            return 0;
        else
            return m_File;
    }

    /** Who's locking the file? */
    size_t getLocker()
    {
        if(m_bLocked)
            return (size_t)(uintptr_t)m_LockerThread;
        else
            return 0;
    }

private:

    /** Default constructor, not to be used */
    LockedFile();

    /** Is a range locked? */

    /** The file that we're wrapping */
    File *m_File;

    /** Is this file locked? */
    bool m_bLocked;

    /** Locker thread address */
    Thread* m_LockerThread;

    /** Our lock */
    Mutex m_Lock;
};

#endif /* _LockedFile_h_ */
