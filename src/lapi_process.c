/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "lapi_process.h"

#include <pipewire/filter.h>

const DSPLoaderAPI lapi_process =
{
    .getportbuffer = (float *(*)(void *, unsigned long))pw_filter_get_dsp_buffer
};

