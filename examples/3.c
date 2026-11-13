/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "dspmodule.h"

#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>

const unsigned short dspmodule_requiredAPIversion = 4;

static unsigned short freq = 0;
static float minvalue = -1, maxvalue = 1, volmod = 0.1;
static void *outport = NULL;

DSPModuleReturnCode dspmodule_startup(const DSPLoaderAPI * restrict lapi, DSPModuleStartupFunctionOptions * restrict opts)
{
    {
        int p;
        while ((p = getopt(opts->argc, opts->argv, "f:m:M:v:")) != -1)
        {
            switch (p)
            {
                case 'f':
                    if (sscanf(optarg, "%hu", &freq) < 1) goto parseoptfail;
                    break;

                case 'm':
                    if (sscanf(optarg, "%f", &minvalue) < 1) goto parseoptfail;
                    break;

                case 'M':
                    if (sscanf(optarg, "%f", &maxvalue) < 1) goto parseoptfail;
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &volmod) < 1) goto parseoptfail;
                    break;

                parseoptfail:
                    lapi->log(DSPLogLevel_Fatal, "failed parsing option -%c", (char)p);
                return 1;
            }
        }
    }

    if (!freq || freq > 20000) { lapi->log(DSPLogLevel_Fatal, "frequency must be in range [1..20000]"); return 1; }

    if (!(outport = lapi->addport("output", NULL, DSPPortDirection_Output, 0))) { lapi->log(DSPLogLevel_Fatal, "error adding output port"); return 1; }

    lapi->log(DSPLogLevel_Info, "frequency: %hu Hz\nmin value: %f\nmax value: %f\nvolmod: %f", freq, minvalue, maxvalue, volmod);
    opts->sysname = "sqwavegen";
    opts->dispname = "square wave generator";
    return 0;
}

DSPModuleReturnCode dspmodule_process(const DSPLoaderAPI * restrict lapi, DSPModuleProcessFunctionOptions * restrict opts)
{
    float *out = lapi->getportbuffer(outport, opts->duration);
    if (!(out && opts->rate)) return 0;

    unsigned long long insecondpos = opts->position % opts->rate;
    unsigned long long fragscount = opts->rate / (2 * freq);

    for (unsigned long i = 0; i < opts->duration; i++) out[i] = (((insecondpos + i) / fragscount) % 2 ? maxvalue : minvalue) * volmod;

    return 0;
}