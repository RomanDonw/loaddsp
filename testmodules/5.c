#include "dspmodule.h"

#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

static unsigned short freq = 0;
static float volmod = 0.5, lastsample;

static inline float rndf(void);

unsigned short dspmodule_startup(const char **name, unsigned short *inportscount, unsigned short *outportscount, int argc, char * const argv[])
{
    {
        int p;
        while ((p = getopt(argc, argv, "f:v:")) != -1)
        {
            switch (p)
            {
                case 'f':
                    if (sscanf(optarg, "%hu", &freq) < 1) { puts("error parsing option -f"); return 1; }
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &volmod) < 1) { puts("error parsing option -v"); return 1; }
                    break;
            }
        }
    }

    srand(time(NULL));

    lastsample = rndf() * volmod;

    *name = "noise generator";
    *inportscount = 0;
    *outportscount = 1;
    return 0;
}

unsigned short dspmodule_process(const float * const inbuffers[], float * const outbuffers[], unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime)
{
    if (!(outbuffers[0] && rate)) return 0;

    if (freq)
    {
        unsigned long long halfrate = rate / 2;
        register unsigned long long fragscount = rate / (freq > halfrate ? halfrate : freq);

        for (unsigned long i = 0; i < duration; i++)
        {
            if (!((position + i) % fragscount)) lastsample = rndf() * volmod;
            outbuffers[0][i] = lastsample;
        }
    }
    else for (unsigned long i = 0; i < duration; i++) outbuffers[0][i] = rndf() * volmod;

    return 0;
}

void dspmodule_cleanup(void) {}

static inline float rndf(void)
{
    return (rand() / (float)RAND_MAX) * 2 - 1;
}