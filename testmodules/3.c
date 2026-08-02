#include "dspmodule.h"

#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>

static unsigned short freq = 0;
static float minvalue = -1, maxvalue = 1;
static void *outport = NULL;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    {
        int p;
        while ((p = getopt(argc, argv, "f:m:M:")) != -1)
        {
            switch (p)
            {
                case 'f':
                    if (sscanf(optarg, "%hu", &freq) < 1) { puts("error parsing option -f"); return 1; }
                    break;

                case 'm':
                    if (sscanf(optarg, "%f", &minvalue) < 1) { puts("error parsing option -m"); return 1; }
                    break;

                case 'M':
                    if (sscanf(optarg, "%f", &maxvalue) < 1) { puts("error parsing option -M"); return 1; }
                    break;
            }
        }
    }

    if (!freq || freq > 20000) { puts("frequency must be in range [1..20000]"); return 1; }

    if (!(outport = lapi->addport("output", NULL, DSPPortDirection_Output, 0))) { puts("error adding output port"); return 1; }

    printf("frequency: %hu Hz\nmin value: %f\nmax value: %f\n", freq, minvalue, maxvalue);
    *sysname = "sqwavegen";
    *dispname = "square wave generator";
    return 0;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime)
{
    float *out = lapi->getportbuffer(outport, duration);
    if (!(out && rate)) return 0;

    unsigned long long insecondpos = position % rate;
    unsigned long long fragscount = rate / (2 * freq);

    for (unsigned long i = 0; i < duration; i++) out[i] = ((insecondpos + i) / fragscount) % 2 ? maxvalue : minvalue;

    return 0;
}

void dspmodule_cleanup(void) {}