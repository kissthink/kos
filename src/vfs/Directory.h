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
#ifndef _Directory_h_
#define _Directory_h_ 1

#include "vfs/File.h"
#include "util/String.h"
#include "util/RadixTree.h"
#include "util/Output.h"

/** A Directory node. */
class Directory : public File
{
    friend class Filesystem;

public:

    /** Eases the pain of casting, and performs a sanity check. */
    static Directory *fromFile(File *pF)
    {
        if (!pF->isDirectory()) ABORT1("Casting non-directory File to Directory!");
        return reinterpret_cast<Directory*> (pF);
    }

    /** Constructor, creates an invalid directory. */
    Directory();

    /** Copy constructors are hidden - unused! */
private:
    Directory(const Directory &file);
    Directory& operator =(const Directory&);

public:
    /** Constructor, should be called only by a Filesystem. */
    Directory(String name, time_t accessedTime, time_t modifiedTime, time_t creationTime,
              uintptr_t inode, class Filesystem *pFs, size_t size, File *pParent);
    /** Destructor - doesn't do anything. */
    virtual ~Directory();

    /** Returns true if the File is actually a directory. */
    virtual bool isDirectory()
    {return true;}

    /** Returns the n'th child of this directory, or an invalid file. */
    File* getChild(size_t n);

    /** Load the directory's contents into the cache. */
    virtual void cacheDirectoryContents();

public:
    /** Directory contents cache. */
    RadixTree<File*> m_Cache;
    bool m_bCachePopulated;
};

#endif /* _Directory_h_ */
