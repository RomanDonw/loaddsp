#include "dspmodule.h"

#include <stdio.h>
#include <string.h>
#include <getopt.h>

static unsigned short ioportpairs = 0;
static float amplitudemodifier = 0, volumemodifier = 1;

unsigned short dspmodule_startup(const char **name, unsigned short *inportscount, unsigned short *outportscount, int argc, char * const argv[])
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
    printf("I/O ports pairs: %hu\namplitude modifier: %f\nvolume modifier: %f\n", ioportpairs, amplitudemodifier, volumemodifier);

    *name = "amplitude/volume modifier";
    *inportscount = ioportpairs;
    *outportscount = ioportpairs;
    return 0;
}

unsigned short dspmodule_process(const float * const inbuffers[], float * const outbuffers[], unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime)
{
    for (unsigned char ch = 0; ch < ioportpairs; ch++)
    {
        const float *in = inbuffers[ch];
        float *out = outbuffers[ch];

        if (!out) continue;
        if (!in) { memset(out, 0, sizeof(float) * duration); continue; }
        
        for (unsigned long i = 0; i < duration; i++) out[i] = adjf(in[i], amplitudemodifier) * volumemodifier;
    }

    return 0;
}

void dspmodule_cleanup(void) {}