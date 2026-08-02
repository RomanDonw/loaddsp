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

static unsigned short freq = 0;
static float volmod = 0.5, lastsample;
static void *outport;

static inline float rndf(void);

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    {
        int p;
        while ((p = getopt(argc, argv, "f:v:")) != -1)
        {
            switch (p)
            {
                case 'f':
                    if (sscanf(optarg, "%hu", &freq) < 1) { puts("error parsing option -f"); return 1; }
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &volmod) < 1) { puts("error parsing option -v"); return 1; }
                    break;
            }
        }
    }

    srand(time(NULL));
    if (freq) lastsample = rndf() * volmod;

    if (!(outport = lapi->addport("output", NULL, DSPPortDirection_Output, 0))) { puts("error creating output port"); return 1; }

    *sysname = "noisegen";
    *dispname = "noise generator";
    return 0;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime)
{
    float *out = lapi->getportbuffer(outport, duration);
    if (!(out && rate)) return 0;

    if (freq)
    {
        register unsigned long long halfrate = rate / 2;
        register unsigned long long fragscount = rate / (freq > halfrate ? halfrate : freq);

        for (unsigned long i = 0; i < duration; i++)
        {
            if (!((position + i) % fragscount)) lastsample = rndf() * volmod;
            out[i] = lastsample;
        }
    }
    else for (unsigned long i = 0; i < duration; i++) out[i] = rndf() * volmod;

    return 0;
}

void dspmodule_cleanup(void) {}

static inline float rndf(void)
{
    return (rand() / (float)RAND_MAX) * 2 - 1;
}
