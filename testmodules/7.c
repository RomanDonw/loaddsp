#include "dspmodule.h"

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>

static float ampmod = 0, volmod = 0.1;

static int8_t *infiledata;
static size_t infilesize;

unsigned short dspmodule_startup(const char **name, unsigned short *inportscount, unsigned short *outportscount, int argc, char * const argv[])
{
    char *infilepath = NULL;
    {
        int p;
        while ((p = getopt(argc, argv, "a:v:f:")) != -1)
        {
            switch (p)
            {
                case 'a':
                    if (sscanf(optarg, "%f", &ampmod) < 1) { puts("error parsing option -a"); return 1; }
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &volmod) < 1) { puts("error parsing option -v"); return 1; }
                    break;

                case 'f':
                    infilepath = optarg;
                    break;
            }
        }
    }

    if (!infilepath) { puts("specify source audio file through -f parameter"); return 1; }

    FILE *f = fopen(infilepath, "rb");
    if (!f) { puts("unable to open specified file"); return 1; }

    fseek(f, 0, SEEK_END);
    infilesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    infiledata = malloc(infilesize);
    if (!infiledata) { puts("memory allocation failed"); fclose(f); return 1; }

    if (fread(infiledata, 1, infilesize, f) < infilesize || ferror(f))
    {
        puts("error reading file");
        free(infiledata);
        fclose(f);
        return 1;
    }

    fclose(f);

    printf("in file: \"%s\"\nampmod: %f\nvolmod: %f\n", infilepath, ampmod, volmod);
    *name = "looped PCM signed 8-bit mono file player";
    *inportscount = 0;
    *outportscount = 1;
    return 0;
}

unsigned short dspmodule_process(const float * const inbuffers[], float * const outbuffers[], unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime)
{
    if (!outbuffers[0]) return 0;

    for (unsigned long i = 0; i < duration; i++)
    {
        register int8_t v = infiledata[(position + i) % infilesize];
        outbuffers[0][i] = adjf(v > 0 ? v / (float)127 : v / (float)128, ampmod) * volmod;
    }

    return 0;
}

void dspmodule_cleanup(void) {}