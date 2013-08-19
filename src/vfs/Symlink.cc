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

#include "vfs/Symlink.h"
#include "vfs/Filesystem.h"
#include "kern/Debug.h"

Symlink::Symlink() :
  File(), m_pCachedSymlink(0)
{
}

Symlink::Symlink(String name, time_t accessedTime, time_t modifiedTime, time_t creationTime,
                 uintptr_t inode, Filesystem *pFs, size_t size, File *pParent) :
  File(name,accessedTime,modifiedTime,creationTime,inode,pFs,size,pParent),
  m_pCachedSymlink(0)
{
}

Symlink::~Symlink()
{
}

File *Symlink::followLink()
{
  if (m_pCachedSymlink)
    return m_pCachedSymlink;

  char *pBuffer = new char[1024];
  read(0ULL, static_cast<uint64_t>(getSize()), reinterpret_cast<uintptr_t>(pBuffer));
  pBuffer[getSize()] = '\0';
  DBG::outln(DBG::VFS, "Followlink, success");
  m_pCachedSymlink = m_pFilesystem->find(String(pBuffer), m_pParent);
  DBG::outln(DBG::VFS, "Followlink, success2");
  return m_pCachedSymlink;
}

int Symlink::followLink(char *pBuffer, size_t bufLen)
{
  return read(0ULL, static_cast<uint64_t>(bufLen), reinterpret_cast<uintptr_t>(pBuffer));
}
