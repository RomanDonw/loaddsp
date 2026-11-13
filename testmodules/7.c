/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "dspmodule.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include <stdbool.h>

const unsigned short dspmodule_requiredAPIversion = 4;

static float ampmod = 0, volmod = 0.1;
static void *infiledata, *outport;
static size_t infilesize;
static bool useu8 = false;

DSPModuleReturnCode dspmodule_startup(const DSPLoaderAPI * restrict lapi, DSPModuleStartupFunctionOptions * restrict opts)
{
    char *infilepath = NULL;
    {
        int p;
        while ((p = getopt(opts->argc, opts->argv, "a:v:f:u")) != -1)
        {
            switch (p)
            {
                case 'a':
                    if (sscanf(optarg, "%f", &ampmod) < 1) goto parseoptfail;
                    break;

                case 'v':
                    if (sscanf(optarg, "%f", &volmod) < 1) goto parseoptfail;
                    break;

                case 'f':
                    infilepath = optarg;
                    break;

                case 'u':
                    useu8 = true;
                    break;

                parseoptfail:
                    lapi->log(DSPLogLevel_Fatal, "failed parsing option -%c", (char)p);
                return 1;
            }
        }
    }

    if (!infilepath) { lapi->log(DSPLogLevel_Fatal, "specify source audio file through -f parameter"); return 1; }

    if (!(outport = lapi->addport("output", NULL, DSPPortDirection_Output, 0))) { lapi->log(DSPLogLevel_Fatal, "error creating output port"); return 1; }

    FILE *f = fopen(infilepath, "rb");
    if (!f) { lapi->log(DSPLogLevel_Fatal, "unable to open specified file"); return 1; }

    fseek(f, 0, SEEK_END);
    infilesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    infiledata = malloc(infilesize);
    if (!infiledata) { lapi->log(DSPLogLevel_Fatal, "memory allocation failed"); fclose(f); return 1; }

    if (fread(infiledata, 1, infilesize, f) < infilesize || ferror(f))
    {
        lapi->log(DSPLogLevel_Fatal, "failed reading file");
        free(infiledata);
        fclose(f);
        return 1;
    }

    fclose(f);

    lapi->log(DSPLogLevel_Info, "in file: \"%s\"\nampmod: %f\nvolmod: %f\nused 8-bit %ssigned PCM", infilepath, ampmod, volmod, useu8 ? "un" : "");
    opts->sysname = "pcmfileplayer";
    opts->dispname = "looped PCM (un)signed 8-bit mono file player";
    return 0;
}

DSPModuleReturnCode dspmodule_process(const DSPLoaderAPI * restrict lapi, DSPModuleProcessFunctionOptions * restrict opts)
{
    float *out = lapi->getportbuffer(outport, opts->duration);
    if (!out) return 0;

    for (unsigned long i = 0; i < opts->duration; i++)
    {
        if (useu8) out[i] = adjf(((uint8_t *)infiledata)[(opts->position + i) % infilesize] / (float)255, ampmod) * volmod;
        else
        {
            register int8_t v = ((int8_t *)infiledata)[(opts->position + i) % infilesize];
            out[i] = adjf(v > 0 ? v / (float)127 : v / (float)128, ampmod) * volmod;
        }
    }
    return 0;
}

void dspmodule_cleanup(void)
{
    free(infiledata);
}
