#ifndef DSPMODULE_H
#define DSPMODULE_H

#include <stdbool.h>
#include <stddef.h>

#define DSPMODULE_API __attribute__((visibility("default")))

enum DSPPortDirection
{
    DSPPortDirection_Input = 0,
    DSPPortDirection_Output = 1
} typedef DSPPortDirection;

struct DSPLoaderAPI
{
    const char *(*getfilterdispname)(void);
    void (*setfilterdispname)(const char *);

    void *(*addport)(const char *, const char *, DSPPortDirection, size_t); // returns NULL on error. second & forth parameters can be zero.
    void (*removeport)(void *);
    const char *(*getportdispname)(void *);
    void (*setportdispname)(void *, const char *);
    float *(*getportbuffer)(void *, unsigned long); // returns NULL if buffer isn't available.
} typedef DSPLoaderAPI;

typedef unsigned short DSPModuleStartupFunctionPrototype(const DSPLoaderAPI *, int argc, char * const argv[], const char **sysname, const char **dispname);
typedef unsigned short DSPModuleProcessFunctionPrototype(const DSPLoaderAPI *, unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime);
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

static inline float adjf(float in, float adj) // adjunctf.
{
    if (!adj) return in;
    if (adj < 0 && absf(in) < absf(adj)) return 0;
    return in + adj * signf(in);
}

#endif