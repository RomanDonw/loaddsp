#include "dspmodule.h"

#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

static unsigned short outportscount = 0;
static void *inport, **outports = NULL;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    {
        int p;
        while ((p = getopt(argc, argv, "o:")) != -1)
        {
            switch (p)
            {
                case 'o':
                    if (sscanf(optarg, "%hu", &outportscount) < 1) { puts("error parsing option -o"); return 1; }
                    break;
            }
        }
    }

    if (!outportscount) { puts("specify at least one output port through -o parameter"); return 1; }

    if (!(inport = lapi->addport("input", NULL, DSPPortDirection_Input, 0))) { puts("error create input port"); return 1; }

    if (!(outports = malloc(outportscount * sizeof(void *)))) { puts("memory allocation failed"); return 1; }

    char namebuff[16];
    for (unsigned short i = 0; i < outportscount; i++)
    {
        if (snprintf(namebuff, sizeof(namebuff), "output_%hu", i) < 0)
        { puts("snprintf formatting error"); goto errorquit_afteralloc; }
        
        if (!(outports[i] = lapi->addport(namebuff, NULL, DSPPortDirection_Output, 0)))
        { printf("unable to create output port with name \"%s\"\n", namebuff); goto errorquit_afteralloc; }
    }

    printf("Output ports: %hu\n", outports);
    *sysname = "multitarget";
    return 0;

    errorquit_afteralloc:
        free(outports);
    return 1;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime)
{
    register const float *in = lapi->getportbuffer(inport, duration);
    for (unsigned short i = 0; i < outportscount; i++)
    {
        register float *out = lapi->getportbuffer(outports[i], duration);
        if (!out) continue;
        if (!in) { memset(out, 0, sizeof(float) * duration); continue; }

        memcpy(out, in, sizeof(float) * duration);
    }

    return 0;
}

void dspmodule_cleanup(void)
{
    free(outports);
}