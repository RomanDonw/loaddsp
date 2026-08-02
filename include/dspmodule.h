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
    const char *(*getfiltersysname)(void);
    const char *(*getfilterdispname)(void);
    void (*setfilterdispname)(const char *dispname);

    // [addport]: returns NULL on error. 'dispname' can be NULL and 'userdatasize' can be equal to zero.
    void *(*addport)(const char *sysname, const char *dispname, DSPPortDirection dir, size_t userdatasize);
    void (*removeport)(void *port);
    const char *(*getportsysname)(void *port);
    const char *(*getportdispname)(void *port);
    void (*setportdispname)(void *port, const char *dispname);
    float *(*getportbuffer)(void *port, unsigned long samplescount); // returns NULL if buffer isn't available.
} typedef DSPLoaderAPI;

typedef unsigned short DSPModuleStartupFunctionPrototype(const DSPLoaderAPI *, int argc, char * const argv[], const char **sysname, const char **dispname);
typedef unsigned short DSPModuleProcessFunctionPrototype(const DSPLoaderAPI *, unsigned long long position, unsigned long long duration, unsigned long rate, unsigned long long nsectime);
typedef void DSPModuleCleanupFunctionPrototype(void);

// [dspmodule_startup]: 'dispname' is not required to be set.
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