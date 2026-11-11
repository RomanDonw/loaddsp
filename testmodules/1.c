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

const unsigned short dspmodule_requiredAPIversion = 4;

static unsigned short ioportpairs = 0;
static float amplitudemodifier = 0, volumemodifier = 1;
static void **inports = NULL, **outports = NULL;

unsigned short dspmodule_startup(const DSPLoaderAPI * restrict lapi, DSPModuleStartupFunctionOptions * restrict opts)
{
    {
        int p;
        while ((p = getopt(opts->argc, opts->argv, "a:p:v:")) != -1)
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

                parseoptfail:
                    lapi->log(DSPLogLevel_Fatal, "failed parsing option -%c", (char)p);
                return 1;
            }
        }
    }

    if (!ioportpairs) { lapi->log(DSPLogLevel_Fatal, "specify at least one I/O ports pair through -p parameter"); return 1; }

    {
        size_t portarrsize = ioportpairs * sizeof(void *);
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

    lapi->log(DSPLogLevel_Info, "I/O ports pairs: %hu\namplitude modifier: %f\nvolume modifier: %f", ioportpairs, amplitudemodifier, volumemodifier);
    opts->sysname = "ampvolmod";
    opts->dispname = "amplitude/volume modifier";
    return 0;

    errorquit_onorafterallocportarrays:
        free(outports);
        free(inports);
    return 1;
}

unsigned short dspmodule_process(const DSPLoaderAPI * restrict lapi, DSPModuleProcessFunctionOptions * restrict opts)
{
    for (unsigned short ch = 0; ch < ioportpairs; ch++)
    {
        float *out = lapi->getportbuffer(outports[ch], opts->duration);
        if (!out) continue;
        const float *in = lapi->getportbuffer(inports[ch], opts->duration);
        if (!in) { memset(out, 0, sizeof(float) * opts->duration); continue; }

        for (unsigned long i = 0; i < opts->duration; i++) out[i] = adjf(in[i], amplitudemodifier) * volumemodifier;
    }

    return 0;
}

void dspmodule_cleanup(void)
{
    free(outports);
    free(inports);
}
