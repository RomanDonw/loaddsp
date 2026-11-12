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

static unsigned short outportscount = 0;
static void *inport, **outports = NULL;

unsigned short dspmodule_startup(const DSPLoaderAPI * restrict lapi, DSPModuleStartupFunctionOptions * restrict opts)
{
    {
        int p;
        while ((p = getopt(opts->argc, opts->argv, "o:")) != -1)
        {
            switch (p)
            {
                case 'o':
                    if (sscanf(optarg, "%hu", &outportscount) < 1) { lapi->log(DSPLogLevel_Fatal, "failed parsing option -o"); return 1; }
                    break;
            }
        }
    }

    if (!outportscount) { lapi->log(DSPLogLevel_Fatal, "specify at least one output port through -o parameter"); return 1; }

    if (!(inport = lapi->addport("input", NULL, DSPPortDirection_Input, 0)))
    { lapi->log(DSPLogLevel_Fatal, "error create input port"); return 1; }

    if (!(outports = malloc(outportscount * sizeof(void *)))) { lapi->log(DSPLogLevel_Fatal, "memory allocation failed"); return 1; }

    char namebuff[16];
    for (unsigned short i = 0; i < outportscount; i++)
    {
        if (snprintf(namebuff, sizeof(namebuff), "output_%hu", i) < 0)
        { lapi->log(DSPLogLevel_Fatal, "snprintf formatting error"); goto errorquit_afteralloc; }
        
        if (!(outports[i] = lapi->addport(namebuff, NULL, DSPPortDirection_Output, 0)))
        { lapi->log(DSPLogLevel_Fatal, "unable to create output port with name \"%s\"", namebuff); goto errorquit_afteralloc; }
    }

    lapi->log(DSPLogLevel_Info, "Output ports: %hu", outportscount);
    opts->sysname = "multitarget";
    return 0;

    errorquit_afteralloc:
        free(outports);
    return 1;
}

unsigned short dspmodule_process(const DSPLoaderAPI * restrict lapi, DSPModuleProcessFunctionOptions * restrict opts)
{
    const float *in = lapi->getportbuffer(inport, opts->duration);
    for (unsigned short i = 0; i < outportscount; i++)
    {
        float *out = lapi->getportbuffer(outports[i], opts->duration);
        if (!out) continue;
        if (!in) { memset(out, 0, sizeof(float) * opts->duration); continue; }

        memcpy(out, in, sizeof(float) * opts->duration);
    }

    return 0;
}

void dspmodule_cleanup(void)
{
    free(outports);
}
