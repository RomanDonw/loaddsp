#include "dspmodule.h"

#include <string.h>
#include <stdio.h>
#include <getopt.h>

static unsigned short ioportpairs = 0;
static float clippingmodifier = 0.5, volumemodifier = 0.1;

unsigned short dspmodule_startup(const char **name, unsigned short *inportscount, unsigned short *outportscount, int argc, char * const argv[])
{
    {
        int p;
        while ((p = getopt(argc, argv, "c:v:p:")) != -1)
        {
            switch (p)
            {
                case 'c':
                    if (sscanf(optarg, "%f", &clippingmodifier) < 1) { puts("error parsing option -c"); return 1; }
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
    if (clippingmodifier < 0) { puts("clipping modifier (-c parameter) cant be less than zero"); return 1; }

    printf("I/O ports pairs: %hu\nclipping modifier: %f\nvolume modifier: %f\n", ioportpairs, clippingmodifier, volumemodifier);

    *name = "distortion effect";
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

        for (unsigned long i = 0; i < duration; i++) out[i] = clampf(in[i] + clippingmodifier * signf(in[i]), -1, 1) * volumemodifier;
    }

    return 0;
}

void dspmodule_cleanup(void) {}