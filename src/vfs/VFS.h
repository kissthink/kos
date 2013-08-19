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
#ifndef _VFS_h
#define _VFS_h 1

#include "kern/Kernel.h"
#include "util/String.h"
#include "Filesystem.h"
#include <list>

/** This class implements a virtual file system.
 *
 * The pedigree VFS is structured in a similar way to windows' - every filesystem
 * is identified by a unique name and accessed thus:
 *
 * myfs:/mydir/myfile
 *
 * No UNIX-style mounting of filesystems inside filesystems is possible.
 * A filesystem may be referred to by multiple names - a reference count is
 * maintained by the filesystem - when no aliases point to it, it is unmounted totally.
 *
 * The 'root' filesystem - that is the FS with system data on, is visible by the alias
 * 'root', thus; 'root:/System/Boot/kernel' could be used to access the kernel image.
 */
class VFS
{
  VFS() = delete;
  VFS(const VFS&) = delete;
  VFS& operator=(const VFS&) = delete;
public:
  /** Callback type, called when a disk is mounted or unmounted. */
  typedef void (*MountCallback)();

  /** Mounts a Disk device as the alias "alias".
      If alias is zero-length, the Filesystem is asked for its preferred name
      (usually a volume name of some sort), and returned in "alias" */
  static bool mount(Disk *pDisk, String &alias);

  /** Adds an alias to an existing filesystem.
   *\param pFs The filesystem to add an alias for.
   *\param pAlias The alias to add. */
  static void addAlias(Filesystem *pFs, String alias);
  static void addAlias(String oldAlias, String newAlias);

  /** Gets a unique alias for a filesystem. */
  static String getUniqueAlias(String alias);

  /** Does a given alias exist? */
  static bool aliasExists(String alias);

  /** Obtains a list of all filesystem aliases */
  static inline RadixTree<Filesystem*> &getAliases() {
    return m_Aliases;
  }

  /** Obtains a list of all mounted filesystems */
  static inline Tree<Filesystem *, std::list<String*,KernelAllocator<String*>>* > &getMounts() {
    return m_Mounts;
  }

  /** Removes an alias from a filesystem. If no aliases remain for that filesystem,
   *  the filesystem is destroyed.
   *\param pAlias The alias to remove. */
  static void removeAlias(String alias);

  /** Removes all aliases from a filesystem - the filesystem is destroyed.
   *\param pFs The filesystem to destroy. */
  static void removeAllAliases(Filesystem *pFs);

  /** Looks up the Filesystem from a given alias.
   *\param pAlias The alias to search for.
   *\return The filesystem aliased by pAlias or 0 if none found. */
  static Filesystem *lookupFilesystem(String alias);

  /** Attempts to obtain a File for a specific path. */
  static File *find(String path, File *pStartNode=0);

  /** Attempts to create a file. */
  static bool createFile(String path, uint32_t mask, File *pStartNode=0);

  /** Attempts to create a directory. */
  static bool createDirectory(String path, File *pStartNode=0);

  /** Attempts to create a symlink. */
  static bool createSymlink(String path, String value, File *pStartNode=0);

  /** Attempts to remove a file/directory/symlink. WILL FAIL IF DIRECTORY NOT EMPTY */
  static bool remove(String path, File *pStartNode=0);

  /** Adds a filesystem probe callback - this is called when a device is mounted. */
  static void addProbeCallback(Filesystem::ProbeCallback callback);

  /** Adds a mount callback - the function is called when a disk is mounted or
      unmounted. */
  static void addMountCallback(MountCallback callback);

private:
  /** A static File object representing an invalid file */
  static File* m_EmptyFile;

private:
  static RadixTree<Filesystem*> m_Aliases;
  static Tree<Filesystem*, std::list<String*,KernelAllocator<String*>>* > m_Mounts;

  static std::list<Filesystem::ProbeCallback*,KernelAllocator<Filesystem::ProbeCallback*>> m_ProbeCallbacks;
  static std::list<MountCallback*,KernelAllocator<MountCallback*>> m_MountCallbacks;
};

#endif /* _VFS_h_ */
