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
#include <math.h>

const unsigned short dspmodule_requiredAPIversion = 3;

static float ampmod = 0, volmod = 0.1, freq = 0, phase = 0;
static void *outport = NULL;

#define PI 3.14159265358979323846

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    {
        int p;
        while ((p = getopt(argc, argv, "f:a:v:p:")) != -1)
        {
            switch (p)
            {
                case 'f':
                    if (sscanf(optarg, "%f", &freq) < 1) goto parseoptfail;
                    break;

                case 'a':
                    if (sscanf(optarg, "%f", &ampmod) < 1) goto parseoptfail;
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &volmod) < 1) goto parseoptfail;
                    break;
                
                case 'p':
                    if (sscanf(optarg, "%f", &phase) < 1) goto parseoptfail;
                    break;

                parseoptfail:
                    lapi->log(DSPLogLevel_Fatal, "failed parsing option -%c", (char)p);
                return 1;
            }
        }
    }

    if (!(outport = lapi->addport("output", NULL, DSPPortDirection_Output, 0)))
    { lapi->log(DSPLogLevel_Fatal, "error adding output port"); return 1; }

    lapi->log(DSPLogLevel_Info, "frequency: %f Hz\nphase: %f seconds\nampmod: %f\nvolmod: %f", freq, phase, ampmod, volmod);
    *sysname = "sinwavegen";
    *dispname = "sine wave generator";
    return 0;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long duration, unsigned long rate, unsigned long long nsectime)
{
    float *out = lapi->getportbuffer(outport, duration);
    if (!out) return 0;

    for (unsigned long i = 0; i < duration; i++)
    {
        double time = (position + (unsigned long long)i) / (double)rate;
        out[i] = adjf(sin((freq * time + phase) * 2 * PI), ampmod) * volmod;
    }

    return 0;
}