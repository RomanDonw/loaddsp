#include "dspmodule.h"

#include <string.h>
#include <stdio.h>
#include <getopt.h>

static unsigned short outports = 0;

unsigned short dspmodule_startup(const char **name, unsigned short *inportscount, unsigned short *outportscount, int argc, char * const argv[])
{
    {
        int p;
        while ((p = getopt(argc, argv, "o:")) != -1)
        {
            switch (p)
            {
                case 'o':
                    if (sscanf(optarg, "%hu", &outports) < 1) { puts("error parsing option -o"); return 1; }
                    break;
            }
        }
    }

    if (!outports) { puts("specify at least one output port through -o parameter"); return 1; }

    printf("Output ports: %hu\n", outports);

    *name = "multitarget";
    *inportscount = 1;
    *outportscount = outports;
    return 0;
}

unsigned short dspmodule_process(const float * const inbuffers[], float * const outbuffers[], unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime)
{
    for (unsigned short ch = 0; ch < outports; ch++)
    {
        if (!outbuffers[ch]) continue;
        if (!inbuffers[0]) { memset(outbuffers[ch], 0, sizeof(float) * duration); continue; }

        memcpy(outbuffers[ch], inbuffers[0], sizeof(float) * duration);
    }

    return 0;
}

void dspmodule_cleanup(void) {}