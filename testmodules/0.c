#include "dspmodule.h"

#include <string.h>

static const char *modname = "test module";

static inline float clampf(float in, float min, float max)
{
    if (in > max) return max;
    if (in < min) return min;
    return in;
}

static inline float signf(float in)
{
    if (in > 0) return 1;
    if (in < 0) return -1;
    return 0;
}

unsigned short dspmodule_startup(const char **name, unsigned short *inportscount, unsigned short *outportscount, int argc, char * const argv[])
{
    *name = modname;
    *inportscount = 1;
    *outportscount = 1;
    return 0;
}

unsigned short dspmodule_process(const float * const inbuffers[], float * const outbuffers[], unsigned long samplescount)
{
    const float *in = inbuffers[0];
    float *out = outbuffers[0];
    if (!out) return 0;
    if (!in)
    { memset(out, 0, sizeof(float) * samplescount); return 0; }

    for (unsigned long i = 0; i < samplescount; i++)
    {
        float sgn = signf(in[i]);
        float val = clampf(in[i] + 0.5 * sgn, -1, 1) * 0.1;
        out[i] = val;
    }

    return 0;
}

void dspmodule_cleanup(void) {}