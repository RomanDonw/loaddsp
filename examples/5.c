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
#include <stdlib.h>
#include <time.h>

const unsigned short dspmodule_requiredAPIversion = 4;

static unsigned short freq = 0;
static float ampmod = 0, volmod = 0.5, lastsample;
static void *outport;

static inline float rndf(void) { return (rand() / (float)RAND_MAX) * 2 - 1; }

DSPModuleReturnCode dspmodule_startup(const DSPLoaderAPI * restrict lapi, DSPModuleStartupFunctionOptions * restrict opts)
{
    {
        int p;
        while ((p = getopt(opts->argc, opts->argv, "f:v:a:")) != -1)
        {
            switch (p)
            {
                case 'f':
                    if (sscanf(optarg, "%hu", &freq) < 1) goto parseoptfail;
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &volmod) < 1) goto parseoptfail;
                    break;

                case 'a':
                    if (sscanf(optarg, "%f", &ampmod) < 1) goto parseoptfail;
                    break;

                parseoptfail:
                    lapi->log(DSPLogLevel_Fatal, "failed parsing option -%c", (char)p);
                return DSPModuleReturnCode_Stop;
            }
        }
    }

    srand(time(NULL));
    if (freq) lastsample = adjf(rndf(), ampmod) * volmod;

    if (!(outport = lapi->addport("output", NULL, DSPPortDirection_Output, 0)))
    { lapi->log(DSPLogLevel_Fatal, "error creating output port"); return DSPModuleReturnCode_Stop; }

    if (freq) lapi->log(DSPLogLevel_Info, "freq: %hu Hz\nampmod: %f\nvolmod: %f", freq, ampmod, volmod);
    else lapi->log(DSPLogLevel_Info, "freq: (relative to sample rate)\nampmod: %f\nvolmod: %f", ampmod, volmod);
    opts->sysname = "noisegen";
    opts->dispname = "noise generator";
    return DSPModuleReturnCode_Continue;
}

DSPModuleReturnCode dspmodule_process(const DSPLoaderAPI * restrict lapi, DSPModuleProcessFunctionOptions * restrict opts)
{
    float *out = lapi->getportbuffer(outport, opts->duration);
    if (!(out && opts->rate)) return DSPModuleReturnCode_Continue;

    if (freq)
    {
        unsigned long long halfrate = opts->rate / 2;
        unsigned long long fragscount = opts->rate / (freq > halfrate ? halfrate : freq);

        for (unsigned long i = 0; i < opts->duration; i++)
        {
            if (!((opts->position + i) % fragscount)) lastsample = adjf(rndf(), ampmod) * volmod;
            out[i] = lastsample;
        }
    }
    else for (unsigned long i = 0; i < opts->duration; i++) out[i] = adjf(rndf(), ampmod) * volmod;

    return DSPModuleReturnCode_Continue;
}