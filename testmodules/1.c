#include "dspmodule.h"

#include <stdio.h>
#include <string.h>
#include <getopt.h>

static float modifier = 0;

unsigned short dspmodule_startup(const char **name, unsigned short *inportscount, unsigned short *outportscount, int argc, char * const argv[])
{
    {
        int p;
        while ((p = getopt(argc, argv, "m:")) != -1)
        {
            switch (p)
            {
                case 'm':
                    if (sscanf(optarg, "%f", &modifier) < 1) { puts("error parsing option -m"); return 1; }
                    break;
            }
        }
    }

    printf("modifier: %f\n", modifier);

    *name = "amplitude modifier";
    *inportscount = 2;
    *outportscount = 2;
    return 0;
}

unsigned short dspmodule_process(const float * const inbuffers[], float * const outbuffers[], unsigned long samplescount)
{
    for (unsigned char j = 0; j < 2; j++)
    {
        const float *in = inbuffers[j];
        float *out = outbuffers[j];

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