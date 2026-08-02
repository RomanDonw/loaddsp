#include "dspmodule.h"

#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

static unsigned short ioportpairs = 0;
static float maxampcutvalue = 0;
static void **inports = NULL, **outports = NULL;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    {
        int p;
        while ((p = getopt(argc, argv, "a:p:")) != -1)
        {
            switch (p)
            {
                case 'a':
                    if (sscanf(optarg, "%f", &maxampcutvalue) < 1) { puts("error parsing option -a"); return 1; }
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

    printf("I/O ports pairs: %hu\nmax amplitude value to cut: %f\n", ioportpairs, maxampcutvalue);
    *sysname = "cut";
    return 0;

    errorquit_onorafterallocportarrays:
        free(outports);
        free(inports);
    return 1;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime)
{
    for (unsigned short ch = 0; ch < ioportpairs; ch++)
    {
        const float *in = lapi->getportbuffer(inports[ch], duration);
        float *out = lapi->getportbuffer(outports[ch], duration);
        if (!out) continue;
        if (!in) { memset(out, 0, sizeof(float) * duration); continue; }

        for (unsigned long i = 0; i < duration; i++)
        {
            if (in[i] <= maxampcutvalue) out[i] = 0;
            else out[i] = in[i];
        }
    }

    return 0;
}

void dspmodule_cleanup(void)
{
    free(outports);
    free(inports);
}