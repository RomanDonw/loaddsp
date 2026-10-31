/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "dspmodule.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

const unsigned short dspmodule_requiredAPIversion = 3;

static unsigned short ioportpairs = 0;
static float amplitudemodifier = 0, volumemodifier = 1, placeholdervalue = 0;
static unsigned long denom = 0;
static void **inports = NULL, **outports = NULL;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    {
        int p;
        while ((p = getopt(argc, argv, "a:p:v:d:h:")) != -1)
        {
            switch (p)
            {
                case 'a':
                    if (sscanf(optarg, "%f", &amplitudemodifier) < 1) goto parseoptfail;
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &volumemodifier) < 1) goto parseoptfail;
                    break;

                case 'p':
                    if (sscanf(optarg, "%hu", &ioportpairs) < 1) goto parseoptfail;
                    break;

                case 'd':
                    if (sscanf(optarg, "%lu", &denom) < 1) goto parseoptfail;
                    break;

                case 'h':
                    if (sscanf(optarg, "%f", &placeholdervalue) < 1) goto parseoptfail;
                    break;

                parseoptfail:
                    lapi->log(DSPLogLevel_Fatal, "failed parsing option -%c", (char)p);
                return 1;
            }
        }
    }

    if (!ioportpairs) { lapi->log(DSPLogLevel_Fatal, "specify at least one I/O ports pair through -p parameter"); return 1; }
    if (!denom) { lapi->log(DSPLogLevel_Fatal, "denominator must be greater/not equal to 0"); return 1; }

    {
        register size_t portarrsize = ioportpairs * sizeof(void *);
        if (!((inports = malloc(portarrsize)) &&
                (outports = malloc(portarrsize))))
        { lapi->log(DSPLogLevel_Fatal, "memory allocation failed"); goto errorquit_onorafterallocportarrays; }
    }

    {
        char namebuff[16];
        for (unsigned short i = 0; i < ioportpairs; i++)
        {
            if (snprintf(namebuff, sizeof(namebuff), "input_%hu", i) < 0)
            { lapi->log(DSPLogLevel_Fatal, "snprintf formatting error"); goto errorquit_onorafterallocportarrays; }
            if (!(inports[i] = lapi->addport(namebuff, NULL, DSPPortDirection_Input, 0)))
            { lapi->log(DSPLogLevel_Fatal, "unable to create input port with name \"%s\"", namebuff); goto errorquit_onorafterallocportarrays; }

            if (snprintf(namebuff, sizeof(namebuff), "output_%hu", i) < 0)
            { lapi->log(DSPLogLevel_Fatal, "snprintf formatting error"); goto errorquit_onorafterallocportarrays; }
            if (!(outports[i] = lapi->addport(namebuff, NULL, DSPPortDirection_Output, 0)))
            { lapi->log(DSPLogLevel_Fatal, "unable to create output port with name \"%s\"", namebuff); goto errorquit_onorafterallocportarrays; }
        }
    }

    lapi->log(DSPLogLevel_Info, "I/O ports pairs: %hu\namplitude modifier: %f\nvolume modifier: %f\nplaceholder value: %f\ndenominator: %lu",
        ioportpairs, amplitudemodifier, volumemodifier, placeholdervalue, denom);
    *sysname = "nsamplesaves";
    *dispname = "n-sample saver"; // idk how correct name it.
    return 0;

    errorquit_onorafterallocportarrays:
        free(outports);
        free(inports);
    return 1;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long duration, unsigned long rate, unsigned long long nsectime)
{
    for (unsigned short ch = 0; ch < ioportpairs; ch++)
    {
        float *out = lapi->getportbuffer(outports[ch], duration);
        if (!out) continue;
        const float *in = lapi->getportbuffer(inports[ch], duration);
        if (!in) { memset(out, 0, sizeof(float) * duration); continue; }
        
        for (unsigned long i = 0; i < duration; i++)
            out[i] = adjf((position + i) % denom ? placeholdervalue : in[i], amplitudemodifier) * volumemodifier;
    }

    return 0;
}

void dspmodule_cleanup(void)
{
    free(outports);
    free(inports);
}
