#include "dspmodule.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include <stdbool.h>

static float ampmod = 0, volmod = 0.1;
static void *infiledata, *outport;
static size_t infilesize;
static bool useu8 = false;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    char *infilepath = NULL;
    {
        int p;
        while ((p = getopt(argc, argv, "a:v:f:u")) != -1)
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

                case 'u':
                    useu8 = true;
                    break;
            }
        }
    }

    if (!infilepath) { puts("specify source audio file through -f parameter"); return 1; }

    if (!(outport = lapi->addport("output", NULL, DSPPortDirection_Output, 0))) { puts("error creating output port"); return 1; }

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

    printf("in file: \"%s\"\nampmod: %f\nvolmod: %f\nused 8-bit %ssigned PCM\n", infilepath, ampmod, volmod, useu8 ? "un" : "");
    *sysname = "pcmfileplayer";
    *dispname = "looped PCM (un)signed 8-bit mono file player";
    return 0;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime)
{
    float *out = lapi->getportbuffer(outport, duration);
    if (!out) return 0;

    for (unsigned long i = 0; i < duration; i++)
    {
        if (useu8)
        {
            register uint8_t v = ((uint8_t *)infiledata)[(position + i) % infilesize];
            out[i] = adjf(v / (float)255, ampmod) * volmod;
        }
        else
        {
            register int8_t v = ((int8_t *)infiledata)[(position + i) % infilesize];
            out[i] = adjf(v > 0 ? v / (float)127 : v / (float)128, ampmod) * volmod;
        }
    }
    return 0;
}

void dspmodule_cleanup(void) {}
