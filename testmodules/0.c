#include "dspmodule.h"

#include <string.h>
#include <stdio.h>

static float clippingmodifier = 0.5, volumemodifier = 0.1;

unsigned short dspmodule_startup(const char **name, unsigned short *inportscount, unsigned short *outportscount, int argc, char * const argv[])
{
    {
        int p;
        while ((p = getopt(argc, argv, "c:v:")) != -1)
        {
            switch (p)
            {
                case 'c':
                    if (sscanf(optarg, "%f", &clippingmodifier) < 1) { puts("error parsing option -c"); return 1; }
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &volumemodifier) < 1) { puts("error parsing option -v"); return 1; }
                    break;
            }
        }
    }

    printf("clipping modifier: %f\nvolume modifier: %f\n", clippingmodifier, volumemodifier);

    *name = "distortion effect";
    *inportscount = 1;
    *outportscount = 1;
    return 0;
}

unsigned short dspmodule_process(const float * const inbuffers[], float * const outbuffers[], unsigned long samplescount)
{
    const float *in = inbuffers[0];
    float *out = outbuffers[0];
    if (!out) return 0;
    if (!in) { memset(out, 0, sizeof(float) * samplescount); return 0; }

    for (unsigned long i = 0; i < samplescount; i++)
    {
        float sgn = signf(in[i]);
        float val = clampf(in[i] + clippingmodifier * sgn, -1, 1) * volumemodifier;
        out[i] = val;
    }

    return 0;
}

void dspmodule_cleanup(void) {}