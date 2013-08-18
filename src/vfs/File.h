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
#ifndef _File_h_
#define _File_h_ 1

#include "kern/Kernel.h"
#include "ipc/BlockingSync.h"
#include "util/String.h"
#include "util/Tree.h"
#include "util/RadixTree.h"
#include "mach/platform.h"

#define FILE_UR 0001
#define FILE_UW 0002
#define FILE_UX 0004
#define FILE_GR 0010
#define FILE_GW 0020
#define FILE_GX 0040
#define FILE_OR 0100
#define FILE_OW 0200
#define FILE_OX 0400

/** A File is a regular file - it is also the superclass of Directory, Symlink
    and Pipe. */
class File
{
    friend class Filesystem;

public:
    /** Constructor, creates an invalid file. */
    File();

    /** Copy constructors are hidden - unused! */
private:
    File(const File &file) = delete;
    File& operator =(const File&) = delete;

public:
    /** Constructor, should be called only by a Filesystem. */
    File(String name, time_t accessedTime, time_t modifiedTime, time_t creationTime,
         uintptr_t inode, class Filesystem *pFs, size_t size, File *pParent);
    /** Destructor - doesn't do anything. */
    virtual ~File();

    /** Reads from the file.
     *  \param[in] buffer Buffer to write the read data into. Can be null, in
     *      which case the data can be found by calling getPhysicalPage.
	 *  \param[in] bCanBlock Whether or not the File can block when reading
	 */
    virtual uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);
    /** Writes to the file.
	 *  \param[in] bCanBlock Whether or not the File can block when reading
	 */
    virtual uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);

    /** Get the physical address for the given offset into the file.
     * Returns (physical_uintptr_t) ~0 if the offset isn't in the cache.
     */
    laddr getPhysicalPage(size_t offset);

    /** Returns the time the file was created. */
    time_t getCreationTime();
    /** Sets the time the file was created. */
    void setCreationTime(time_t t);

    /** Returns the time the file was last accessed. */
    time_t getAccessedTime();
    /** Sets the time the file was last accessed. */
    void setAccessedTime(time_t t);

    /** Returns the time the file was last modified. */
    time_t getModifiedTime();
    /** Sets the time the file was last modified. */
    void setModifiedTime(time_t t);

    /** Returns the name of the file. */
    String getName();
    void getName(String &s);
    // File names cannot be changed.

    /** Obtains the full path of the File. */
    virtual String getFullPath()
    {
        String str;
        kstring fullPath;
        kstring tmp;

        if (getParent() != 0) {
            str = getName();
            fullPath = static_cast<const char*>(str);
        }

        File* f = this;
        while ((f = f->getParent()))
        {
            // This feels a bit weird considering the while loop's subject...
            if (f->getParent())
            {
                tmp = fullPath;
                str = f->getName();
                fullPath = static_cast<const char*>(str);
                fullPath += "/";
                fullPath += tmp;
            }
        }

        tmp = fullPath;
        fullPath = "/";
        fullPath += tmp;

        return String(fullPath.c_str());
    }

    /** Delete all data from the file. */
    virtual void truncate();

    size_t getSize();
    void setSize(size_t sz);

    /** Returns true if the File is actually a symlink. */
    virtual bool isSymlink()
    {
        return false;
    }

    /** Returns true if the File is actually a directory. */
    virtual bool isDirectory()
    {
        return false;
    }

    /** Returns true if the File is actually a pipe. */
    virtual bool isPipe()
    {
        return false;
    }

    uintptr_t getInode()
    {return m_Inode;}
    virtual void setInode(uintptr_t inode)
    {m_Inode = inode;}

    Filesystem *getFilesystem()
    {return m_pFilesystem;}
    void setFilesystem(Filesystem *pFs)
    {m_pFilesystem = pFs;}

    virtual void fileAttributeChanged()
    {}

    virtual void increaseRefCount(bool bIsWriter)
    {
        if (bIsWriter)
            m_nWriters ++;
        else
            m_nReaders ++;
    }

    virtual void decreaseRefCount(bool bIsWriter)
    {
        if (bIsWriter)
            m_nWriters --;
        else
            m_nReaders --;
    }

    void setPermissions(uint32_t perms)
    {m_Permissions = perms;}

    uint32_t getPermissions()
    {return m_Permissions;}

    void setUid(size_t uid)
    {m_Uid = uid;}
    size_t getUid()
    {return m_Uid;}

    void setGid(size_t gid)
    {m_Gid = gid;}
    size_t getGid()
    {return m_Gid;}

    File *getParent()
    {return m_pParent;}

    /** Similar to POSIX's select() function
     * \return 1 if ready for reading/writing, 0 otherwise
     * \note Default implementation says always ready to read/write
     *       so be sure to override if that's not right
     */
    virtual int select(bool bWriting = false, int timeout = 0)
    {return 1;}

#if 0
    /** Causes the event pEvent to be dispatched to pThread when activity occurs
        on this File. Activity includes the file becoming available for reading,
        writing or erroring. */
    void monitor(Thread *pThread, Event *pEvent)
    {
        ScopedLock<Mutex> lo(m_Lock);
        m_MonitorTargets.push_back(new MonitorTarget(pThread, pEvent));
    }

    /** Walks the monitor-target queue, removing all for \p pThread .*/
    void cullMonitorTargets(Thread *pThread);
#endif

protected:

    /** Internal function to retrieve an aligned 512byte section of the file. */
    virtual uintptr_t readBlock(uint64_t location)
    {
        return 0;
    }
    /** Internal function to retrieve the block size returned by readBlock.
        \note This must be constant throughout the life of the file. */
    virtual size_t getBlockSize() const
    {return pagesize<1>(); }

    /** Internal function to extend a file to be at least the given size. */
    virtual void extend(size_t newSize)
    {
        if(m_Size < newSize)
            m_Size = newSize;
    }

    /** Internal function to notify all registered MonitorTargets. */
#if 0
    void dataChanged();
#endif

    String m_Name;
    time_t m_AccessedTime;
    time_t m_ModifiedTime;
    time_t m_CreationTime;
    uintptr_t m_Inode;

    class Filesystem *m_pFilesystem;
    size_t m_Size;

    File *m_pParent;

    size_t m_nWriters, m_nReaders;

    size_t m_Uid;
    size_t m_Gid;
    uint32_t m_Permissions;

    Tree<size_t,size_t> m_DataCache;

    Mutex m_Lock;

#if 0
    struct MonitorTarget
    {
        MonitorTarget(Thread *pT, Event *pE) :
            pThread(pT), pEvent(pE)
        {}
        Thread *pThread;
        Event  *pEvent;
    };
#endif

//    std::list<MonitorTarget*,KernelAllocator<MonitorTarget*>> m_MonitorTargets;
};

#endif
