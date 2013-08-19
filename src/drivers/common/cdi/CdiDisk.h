/*
* Copyright (c) 2009 Kevin Wolf <kevin@tyndur.org>
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
#ifndef _CDI_COMMON_Disk_h_
#define _CDI_COMMON_Disk_h_ 1

#include "mach/Device.h"
#include "mach/Disk.h"
#include "mach/IOBase.h"
#include "mach/IOPort.h"
#include "mach/MemoryRegion.h"
#include "mach/IrqHandler.h"
#include "kern/Thread.h"
#include "ipc/BlockingSync.h"
#include "util/Cache.h"
#include "util/String.h"

#include "cdi/storage.h"

/** CDI Disk Device */
class CdiDisk : public Disk
{
    public:
        CdiDisk(Disk* pDev, struct cdi_storage_device* device);
        virtual ~CdiDisk();

        virtual void getName(String &str)
        {
            // TODO Get the name from the CDI driver
            if(!m_Device)
                str = "cdi-disk";
            else
            {
                str = String(m_Device->dev.name);
            }
        }

        /** Tries to detect if this device is present.
         * \return True if the device is present and was successfully initialised. */
        bool initialise();

        // These are the functions that others call - they add a request to the parent controller's queue.
        virtual uintptr_t read(uint64_t location);
        virtual void write(uint64_t location);

        /// Assume CDI-provided disks are never read-only.
        virtual bool cacheIsCritical()
        {
            return false;
        }

        /// CDI disks do not yet do any form of caching.
        /// \todo Fix that.
        virtual void flush(uint64_t location)
        {
            return;
        }

    private:
        CdiDisk(const CdiDisk&);
        const CdiDisk & operator = (const CdiDisk&);

        struct cdi_storage_device* m_Device;
        Cache m_Cache;
};

#endif /*_CDI_COMMON_Disk_h_ */
