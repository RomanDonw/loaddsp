#include "dspmodule.h"

#include <string.h>
#include <stdio.h>
#include <getopt.h>

static unsigned short ioportpairs = 0;
static float minvalue = -1, maxvalue = 1;

unsigned short dspmodule_startup(const char **name, unsigned short *inportscount, unsigned short *outportscount, int argc, char * const argv[])
{
    {
        int p;
        while ((p = getopt(argc, argv, "m:M:p:")) != -1)
        {
            switch (p)
            {
                case 'm':
                    if (sscanf(optarg, "%f", &minvalue) < 1) { puts("error parsing option -m"); return 1; }
                    break;

                case 'M':
                    if (sscanf(optarg, "%f", &maxvalue) < 1) { puts("error parsing option -M"); return 1; }
                    break;

                case 'p':
                    if (sscanf(optarg, "%hu", &ioportpairs) < 1) { puts("error parsing option -p"); return 1; }
                    break;
            }
        }
    }

    if (!ioportpairs) { puts("specify at least one I/O ports pair through -p parameter"); return 1; }

    printf("I/O ports pairs: %hu\nminimum value: %f\nmaximum value: %f\n", ioportpairs, minvalue, maxvalue);

    *name = "clamp";
    *inportscount = ioportpairs;
    *outportscount = ioportpairs;
    return 0;
}

unsigned short dspmodule_process(const float * const inbuffers[], float * const outbuffers[], unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime)
{
    for (unsigned short ch = 0; ch < ioportpairs; ch++)
    {
        const float *in = inbuffers[ch];
        float *out = outbuffers[ch];
        if (!out) continue;
        if (!in) { memset(out, 0, sizeof(float) * duration); continue; }

        for (unsigned long i = 0; i < duration; i++) out[i] = clampf(in[i], minvalue, maxvalue);
    }

    return 0;
}

void dspmodule_cleanup(void) {}