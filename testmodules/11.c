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

const unsigned short dspmodule_requiredAPIversion = 3;

static unsigned short inportscount = 0;
static void **inports = NULL, *outport;
static float **inbuffers = NULL;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    {
        int p;
        while ((p = getopt(argc, argv, "p:")) != -1)
        {
            switch (p)
            {
                case 'p':
                    if (sscanf(optarg, "%hu", &inportscount) < 1) lapi->log(DSPLogLevel_Fatal, "failed parsing option -p");
                    break;
            }
        }
    }

    if (inportscount < 2) { lapi->log(DSPLogLevel_Fatal, "specify at least two in ports through -p parameter"); return 1; }

    if (!(outport = lapi->addport("output", NULL, DSPPortDirection_Output, 0)))
    { lapi->log(DSPLogLevel_Fatal, "unable to create single output port"); return 1; }

    if (!(inports = malloc(inportscount * sizeof(void *))) || 
        !(inbuffers = malloc(inportscount * sizeof(float *))))
    { lapi->log(DSPLogLevel_Fatal, "memory allocation failed"); goto errorquit_freeinports; }

    char namebuff[16];
    for (unsigned short i = 0; i < inportscount; i++)
    {
        if (snprintf(namebuff, sizeof(namebuff), "input_%hu", i) < 0)
        { lapi->log(DSPLogLevel_Fatal, "snprintf formatting error"); goto errorquit_freeinports; }
        if (!(inports[i] = lapi->addport(namebuff, NULL, DSPPortDirection_Input, 0)))
        { lapi->log(DSPLogLevel_Fatal, "unable to create input port with name \"%s\"", namebuff); goto errorquit_freeinports; }
    }

    lapi->log(DSPLogLevel_Info, "inportscount: %hu", inportscount);
    *sysname = "monomixer";
    *dispname = "mono mixer";
    return 0;

    errorquit_freeinports:
        free(inbuffers);
        free(inports);
    return 1;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long duration, unsigned long rate, unsigned long long nsectime)
{
    float *out = lapi->getportbuffer(outport, duration);
    if (!out) return 0;

    unsigned short realcount = 0;
    for (unsigned short i = 0; i < inportscount; i++)
    {
        float *buff = lapi->getportbuffer(inports[i], duration);
        if (buff) inbuffers[realcount++] = buff;
    }
    if (!realcount) { memset(out, 0, duration * sizeof(float)); return 0; }

    for (unsigned long i = 0; i < duration; i++)
    {
        float v = 0;
        for (unsigned short j = 0; j < realcount; j++) v += inbuffers[j][i];
        out[i] = v / (float)realcount;
    }

    return 0;
}

void dspmodule_cleanup(void)
{
    free(inbuffers);
    free(inports);
}