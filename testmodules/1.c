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

static unsigned short ioportpairs = 0;
static float amplitudemodifier = 0, volumemodifier = 1;
static void **inports = NULL, **outports = NULL;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    {
        int p;
        while ((p = getopt(argc, argv, "a:p:v:")) != -1)
        {
            switch (p)
            {
                case 'a':
                    if (sscanf(optarg, "%f", &amplitudemodifier) < 1) { puts("error parsing option -a"); return 1; }
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &volumemodifier) < 1) { puts("error parsing option -v"); return 1; }
                    break;

                case 'p':
                    if (sscanf(optarg, "%hu", &ioportpairs) < 1) { puts("error parsing option -p"); return 1; }
                    break;
            }
        }
    }

    if (!ioportpairs) { puts("specify at least one I/O ports pair through -p parameter"); return 1; }

    {
        register size_t portarrsize = ioportpairs * sizeof(void *);
        if (!((inports = malloc(portarrsize)) &&
                (outports = malloc(portarrsize))))
        { puts("memory allocation failed"); goto errorquit_onorafterallocportarrays; }
    }

    {
        char namebuff[16];
        for (unsigned short i = 0; i < ioportpairs; i++)
        {
            if (snprintf(namebuff, sizeof(namebuff), "input_%hu", i) < 0)
            { puts("snprintf formatting error"); goto errorquit_onorafterallocportarrays; }
            if (!(inports[i] = lapi->addport(namebuff, NULL, DSPPortDirection_Input, 0)))
            { printf("unable to create input port with name \"%s\"\n", namebuff); goto errorquit_onorafterallocportarrays; }

            if (snprintf(namebuff, sizeof(namebuff), "output_%hu", i) < 0)
            { puts("snprintf formatting error"); goto errorquit_onorafterallocportarrays; }
            if (!(outports[i] = lapi->addport(namebuff, NULL, DSPPortDirection_Output, 0)))
            { printf("unable to create output port with name \"%s\"\n", namebuff); goto errorquit_onorafterallocportarrays; }
        }
    }

    printf("I/O ports pairs: %hu\namplitude modifier: %f\nvolume modifier: %f\n", ioportpairs, amplitudemodifier, volumemodifier);
    *sysname = "ampvolmod";
    *dispname = "amplitude/volume modifier";
    return 0;

    errorquit_onorafterallocportarrays:
        free(outports);
        free(inports);
    return 1;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime)
{
    for (unsigned char ch = 0; ch < ioportpairs; ch++)
    {
        const float *in = lapi->getportbuffer(inports[ch], duration);
        float *out = lapi->getportbuffer(outports[ch], duration);
        if (!out) continue;
        if (!in) { memset(out, 0, sizeof(float) * duration); continue; }
        
        for (unsigned long i = 0; i < duration; i++) out[i] = adjf(in[i], amplitudemodifier) * volumemodifier;
    }

    return 0;
}

void dspmodule_cleanup(void)
{
    free(outports);
    free(inports);
}
