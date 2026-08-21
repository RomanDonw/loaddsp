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
#include <libopenmpt/libopenmpt.h>

static float ampmod = 0, volmod = 0.1;
static void *outportl, *outportr;
static openmpt_module *mod;
static bool quitonsongend = true;

unsigned short dspmodule_startup(const DSPLoaderAPI *lapi, int argc, char * const argv[], const char **sysname, const char **dispname)
{
    char *infilepath = NULL;
    {
        int p;
        while ((p = getopt(argc, argv, "a:v:f:s")) != -1)
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

                case 's':
                    quitonsongend = false;
                    break;
            }
        }
    }

    if (!infilepath) { puts("specify source tracker module file through -f parameter"); return 1; }

    if (!(outportl = lapi->addport("left", NULL, DSPPortDirection_Output, 0))) { puts("error creating output port for left audio channel"); return 1; }
    if (!(outportr = lapi->addport("right", NULL, DSPPortDirection_Output, 0))) { puts("error creating output port for right audio channel)"); return 1; }

    FILE *f = fopen(infilepath, "rb");
    if (!f) { puts("unable to open specified file"); return 1; }

    fseek(f, 0, SEEK_END);
    size_t infilesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    void *infiledata = malloc(infilesize);
    if (!infiledata) { puts("memory allocation failed"); fclose(f); return 1; }

    if (fread(infiledata, 1, infilesize, f) < infilesize || ferror(f))
    {
        puts("error reading file");
        free(infiledata);
        fclose(f);
        return 1;
    }

    fclose(f);

    mod = openmpt_module_create_from_memory2(infiledata, infilesize, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    free(infiledata);
    if (!mod) { puts("error reading passed tracker music module file"); return 1; }

    openmpt_module_set_repeat_count(mod, -1);

    printf("in file: \"%s\"\nampmod: %f\nvolmod: %f\n", infilepath, ampmod, volmod);
    *sysname = "trackmusicplayer";
    *dispname = "looped track music player";
    return 0;
}

unsigned short dspmodule_process(const DSPLoaderAPI *lapi, unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime)       
{
    register float *leftch = lapi->getportbuffer(outportl, duration);
    if (!leftch) return 0;
    register float *rightch = lapi->getportbuffer(outportr, duration);
    if (!rightch) return 0;

    size_t frames = openmpt_module_read_float_stereo(mod, rate, duration, leftch, rightch);
    if (frames < duration)
    {
        memset(&leftch[frames], 0, (duration - frames) * sizeof(float));
        if (quitonsongend) return 1;
    }

    return 0;
}

void dspmodule_cleanup(void)
{
    openmpt_module_destroy(mod);
}
