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
#ifndef _CDI_COMMON_Net_h_
#define _CDI_COMMON_Net_h_ 1

#include "mach/Device.h"
#include "mach/Network.h"
#include "mach/IOBase.h"
#include "mach/IOPort.h"
#include "mach/MemoryRegion.h"
#include "mach/IrqHandler.h"
#include "kern/Thread.h"
#include "ipc/BlockingSync.h"

#include "cdi/net.h"

/** CDI NIC Device */
class CdiNet : public Network
{
    public:
        CdiNet(Network* pDev, struct cdi_net_device* device);
        ~CdiNet();

        virtual void getName(String &str)
        {
            // TODO Get the name from the CDI driver
            if(!m_Device)
                str = "cdi-net";
            else
            {
                str = String(m_Device->dev.name);
            }
        }

        virtual bool send(size_t nBytes, uintptr_t buffer);

        virtual bool setStationInfo(StationInfo info);
        virtual StationInfo getStationInfo();

    private:
        CdiNet(const CdiNet&);
        const CdiNet & operator = (const CdiNet&);

        struct cdi_net_device* m_Device;
};

#endif /* _CDI_COMMON_Net_h_ */
