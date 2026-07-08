#include "dspmodule.h"

#include <string.h>
#include <stdio.h>
#include <getopt.h>

static unsigned short ioportpairs = 0;
static float inamod = 0.5, invmod = 1, minval = -0.5, maxval = 0.5, outamod = 0, outvmod = 0.1;

unsigned short dspmodule_startup(const char **name, unsigned short *inportscount, unsigned short *outportscount, int argc, char * const argv[])
{
    {
        int p;
        while ((p = getopt(argc, argv, "a:v:A:V:m:M:p:")) != -1)
        {
            switch (p)
            {
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

    printf("I/O ports pairs: %hu\ninamod: %f\ninvmod: %f\nminval: %f\nmaxval: %f\noutamod: %f\noutvmod: %f\n", ioportpairs, inamod, invmod, minval, maxval, outamod, outvmod);

    *name = "completed distortion effect";
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

        for (unsigned long i = 0; i < duration; i++) out[i] = adjf(clampf(adjf(in[i], inamod) * invmod, minval, maxval), outamod) * outvmod;
    }

    return 0;
}

void dspmodule_cleanup(void) {}