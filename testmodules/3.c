#include "dspmodule.h"

#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>

static unsigned short freq = 0;
static float minvalue = -1, maxvalue = 1;

unsigned short dspmodule_startup(const char **name, unsigned short *inportscount, unsigned short *outportscount, int argc, char * const argv[])
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

    *name = "square wave generator";
    *inportscount = 0;
    *outportscount = 1;
    return 0;
}

unsigned short dspmodule_process(const float * const inbuffers[], float * const outbuffers[], unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime)
{
    if (!(outbuffers[0] && rate)) return 0;

    unsigned long long insecondpos = position % rate;
    unsigned long long fragscount = rate / (2 * freq);

    for (unsigned long i = 0; i < duration; i++) outbuffers[0][i] = ((insecondpos + i) / fragscount) % 2 ? maxvalue : minvalue;

    return 0;
}

void dspmodule_cleanup(void) {}