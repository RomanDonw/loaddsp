#include "dspmodule.h"

#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

static unsigned short ioportpairs = 0;
static float incutthreshold = 0, inamod = 0.5, invmod = 1, minval = -0.5, maxval = 0.5, outamod = 0, outvmod = 0.1;
static void **inports = NULL, **outports = NULL;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    {
        int p;
        while ((p = getopt(argc, argv, "c:a:v:A:V:m:M:p:")) != -1)
        {
            switch (p)
            {
                case 'c':
                    if (sscanf(optarg, "%f", &incutthreshold) < 1) { puts("error parsing option -a"); return 1; }
                    break;

                case 'a':
                    if (sscanf(optarg, "%f", &inamod) < 1) { puts("error parsing option -a"); return 1; }
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &invmod) < 1) { puts("error parsing option -v"); return 1; }
                    break;

                case 'A':
                    if (sscanf(optarg, "%f", &outamod) < 1) { puts("error parsing option -A"); return 1; }
                    break;

                case 'V':
                    if (sscanf(optarg, "%f", &outvmod) < 1) { puts("error parsing option -V"); return 1; }
                    break;

                case 'm':
                    if (sscanf(optarg, "%f", &minval) < 1) { puts("error parsing option -m"); return 1; }
                    break;

                case 'M':
                    if (sscanf(optarg, "%f", &maxval) < 1) { puts("error parsing option -M"); return 1; }
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

    printf("I/O ports pairs: %hu\nincutthreshold: %f\ninamod: %f\ninvmod: %f\nminval: %f\nmaxval: %f\noutamod: %f\noutvmod: %f\n",
        ioportpairs, incutthreshold, inamod, invmod, minval, maxval, outamod, outvmod);
    *sysname = "distortion";
    *dispname = "complete distortion effect";
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
            out[i] = (incutthreshold >= 0) && absf(in[i]) <= incutthreshold ? 0 :
                adjf(clampf(adjf(in[i], inamod) * invmod, minval, maxval), outamod) * outvmod;
    }

    return 0;
}

void dspmodule_cleanup(void)
{
    free(outports);
    free(inports);
}