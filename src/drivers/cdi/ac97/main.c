/******************************************************************************
 * Copyright (c) 2010 Max Reitz                                               *
 *                                                                            *
 * Permission is hereby granted,  free of charge,  to any person  obtaining a *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction,  including without limitation *
 * the rights to use, copy,  modify, merge, publish,  distribute, sublicense, *
 * and/or  sell copies of the  Software,  and to permit  persons to whom  the *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED,  INCLUDING BUT NOT LIMITED TO THE WARRANTIES  OF MERCHANTABILITY, *
 * FITNESS  FOR A PARTICULAR  PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY,  WHETHER IN AN ACTION OF CONTRACT,  TORT OR OTHERWISE,  ARISING *
 * FROM,  OUT OF  OR IN CONNECTION  WITH  THE SOFTWARE  OR THE  USE OR  OTHER *
 * DEALINGS IN THE SOFTWARE.                                                  *
 ******************************************************************************/

#include "cdi/audio.h"

#include "device.h"

#define DRIVER_NAME "ac97"

static struct cdi_audio_driver driver;

static int ac97_driver_init(void)
{
    cdi_driver_init(&driver.drv);

    return 0;
}

static int ac97_driver_destroy(void)
{
    cdi_driver_destroy(&driver.drv);

    return 0;
}

static struct cdi_audio_driver driver = {
    .drv = {
        .name          = DRIVER_NAME,
        .type          = CDI_AUDIO,
        .bus           = CDI_PCI,
        .init          = ac97_driver_init,
        .destroy       = ac97_driver_destroy,
        .init_device   = ac97_init_device,
        .remove_device = ac97_remove_device
    },

    .transfer_data          = ac97_transfer_data,
    .change_device_status   = ac97_change_status,
    .set_volume             = ac97_set_volume,
    .set_sample_rate        = ac97_set_sample_rate,
    .get_position           = ac97_get_position,
    .set_number_of_channels = ac97_set_channel_count
};

CDI_DRIVER(DRIVER_NAME, driver)

