/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "dspmodule.h"

#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

const unsigned short dspmodule_requiredAPIversion = 4;

static unsigned short ioportpairs = 0;
static float incutthreshold = 0, inamod = 0.5, invmod = 1, minval = -0.5, maxval = 0.5, outamod = 0, outvmod = 0.1;
static void **inports = NULL, **outports = NULL;

DSPModuleReturnCode dspmodule_startup(const DSPLoaderAPI * restrict lapi, DSPModuleStartupFunctionOptions * restrict opts)
{
    {
        int p;
        while ((p = getopt(opts->argc, opts->argv, "c:a:v:A:V:m:M:p:")) != -1)
        {
            switch (p)
            {
                case 'c':
                    if (sscanf(optarg, "%f", &incutthreshold) < 1) goto parseoptfail;
                    break;

                case 'a':
                    if (sscanf(optarg, "%f", &inamod) < 1) goto parseoptfail;
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &invmod) < 1) goto parseoptfail;
                    break;

                case 'A':
                    if (sscanf(optarg, "%f", &outamod) < 1) goto parseoptfail;
                    break;

                case 'V':
                    if (sscanf(optarg, "%f", &outvmod) < 1) goto parseoptfail;
                    break;

                case 'm':
                    if (sscanf(optarg, "%f", &minval) < 1) goto parseoptfail;
                    break;

                case 'M':
                    if (sscanf(optarg, "%f", &maxval) < 1) goto parseoptfail;
                    break;

                case 'p':
                    if (sscanf(optarg, "%hu", &ioportpairs) < 1) goto parseoptfail;
                    break;

                parseoptfail:
                    lapi->log(DSPLogLevel_Fatal, "failed parsing option -%c", (char)p);
                return DSPModuleReturnCode_Stop;
            }
        }
    }

    if (!ioportpairs) { lapi->log(DSPLogLevel_Fatal, "specify at least one I/O ports pair through -p parameter"); return DSPModuleReturnCode_Stop; }

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

    lapi->log(DSPLogLevel_Info, "I/O ports pairs: %hu\nincutthreshold: %f\ninamod: %f\ninvmod: %f\nminval: %f\nmaxval: %f\noutamod: %f\noutvmod: %f",
        ioportpairs, incutthreshold, inamod, invmod, minval, maxval, outamod, outvmod);
    opts->sysname = "distortion";
    opts->dispname = "complete distortion effect";
    return DSPModuleReturnCode_Continue;

    errorquit_onorafterallocportarrays:
        free(outports);
        free(inports);
    return DSPModuleReturnCode_Stop;
}

DSPModuleReturnCode dspmodule_process(const DSPLoaderAPI * restrict lapi, DSPModuleProcessFunctionOptions * restrict opts)
{
    for (unsigned short ch = 0; ch < ioportpairs; ch++)
    {
        float *out = lapi->getportbuffer(outports[ch], opts->duration);
        if (!out) continue;
        const float *in = lapi->getportbuffer(inports[ch], opts->duration);
        if (!in) { memset(out, 0, sizeof(float) * opts->duration); continue; }

        for (unsigned long i = 0; i < opts->duration; i++)
            out[i] = (incutthreshold >= 0) && absf(in[i]) <= incutthreshold ? 0 :
                adjf(clampf(adjf(in[i], inamod) * invmod, minval, maxval), outamod) * outvmod;
    }

    return DSPModuleReturnCode_Continue;
}

void dspmodule_cleanup(void)
{
    free(outports);
    free(inports);
}
