#ifndef DSPMODULE_H
#define DSPMODULE_H

#define DSPMODULE_API __attribute__((visibility("default")))

typedef unsigned short DSPModuleStartupFunctionPrototype(const char **name, unsigned short *inportscount, unsigned short *outportscount, int argc, char * const argv[]);
typedef unsigned short DSPModuleProcessFunctionPrototype(const float * const inbuffers[], float * const outbuffers[], unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime);
typedef void DSPModuleCleanupFunctionPrototype(void);

DSPMODULE_API DSPModuleStartupFunctionPrototype dspmodule_startup;
DSPMODULE_API DSPModuleProcessFunctionPrototype dspmodule_process;
DSPMODULE_API DSPModuleCleanupFunctionPrototype dspmodule_cleanup;

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

#define absf(in) (((float)in) < 0 ? -((float)in) : ((float)in))

#endif