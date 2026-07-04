#include "dspmodule.h"

#include <stdio.h>
#include <string.h>
#include <getopt.h>

static unsigned short ioportpairs = 0;
static float modifier = 0;

unsigned short dspmodule_startup(const char **name, unsigned short *inportscount, unsigned short *outportscount, int argc, char * const argv[])
{
    {
        int p;
        while ((p = getopt(argc, argv, "m:p:")) != -1)
        {
            switch (p)
            {
                case 'm':
                    if (sscanf(optarg, "%f", &modifier) < 1) { puts("error parsing option -m"); return 1; }
                    break;

                case 'p':
                    if (sscanf(optarg, "%hu", &ioportpairs) < 1) { puts("error parsing option -p"); return 1; }
                    break;
            }
        }
    }

    if (!ioportpairs) { puts("specify at least one I/O ports pair through -p parameter"); return 1; }
    printf("I/O ports pairs: %hu\nmodifier: %f\n", ioportpairs, modifier);

    *name = "amplitude modifier";
    *inportscount = ioportpairs;
    *outportscount = ioportpairs;
    return 0;
}

unsigned short dspmodule_process(const float * const inbuffers[], float * const outbuffers[], unsigned long samplescount)
{
    for (unsigned char ch = 0; ch < ioportpairs; ch++)
    {
        const float *in = inbuffers[ch];
        float *out = outbuffers[ch];

        if (!out) continue;
        if (!in) { memset(out, 0, sizeof(float) * samplescount); continue; }
        
        for (unsigned long i = 0; i < samplescount; i++)
        {
            if (modifier < 0 && absf(in[i]) < absf(modifier)) out[i] = 0;
            else out[i] = in[i] + modifier * signf(in[i]);
        }
    }

    return 0;
}

void dspmodule_cleanup(void) {}