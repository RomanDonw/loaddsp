/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef DSPMODULE_H
#define DSPMODULE_H

// Current API version: 4.

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

#define DSPMODULE_API __attribute__((visibility("default")))

enum DSPPortDirection
{
    DSPPortDirection_Input = 0,
    DSPPortDirection_Output = 1
} typedef DSPPortDirection;

enum DSPState
{
    DSPState_Paused = 0,
    DSPState_Streaming = 1
} typedef DSPState;

enum DSPLogLevel
{
    DSPLogLevel_Debug = 0,
    DSPLogLevel_Info = 1,
    DSPLogLevel_Warning = 2,
    DSPLogLevel_Error = 3,
    DSPLogLevel_Fatal = 4
} typedef DSPLogLevel;

typedef void DSPLoaderAPILogFunctionPrototype(DSPLogLevel, const char *format, ...);

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

    DSPLoaderAPILogFunctionPrototype *log;
} typedef DSPLoaderAPI;

struct DSPModuleStartupFunctionOptions
{
    /* === IN === */
    
    int argc;
    char * const *argv;

    /* === OUT === */

    // Required:
    const char *sysname;
    const char *dispname;

    // Optional:
    unsigned long forcerate;
    unsigned long forcequantum;
} typedef DSPModuleStartupFunctionOptions;

struct DSPModuleProcessFunctionOptions
{
    unsigned long long position, nsectime;
    unsigned long duration, rate;
} typedef DSPModuleProcessFunctionOptions;

enum DSPModuleReturnCode
{
    DSPModuleReturnCode_Continue = 0,
    DSPModuleReturnCode_Quit = 1, // success quit (indicates that quit raised not by runtime error, e.g. reached EOF of playing file).
    DSPModuleReturnCode_Stop = 2 // error quit (raised by runtime error).
} typedef DSPModuleReturnCode;

typedef DSPModuleReturnCode DSPModuleStartupFunctionPrototype(const DSPLoaderAPI * restrict, DSPModuleStartupFunctionOptions * restrict);
typedef DSPModuleReturnCode DSPModuleProcessFunctionPrototype(const DSPLoaderAPI * restrict, DSPModuleProcessFunctionOptions * restrict);
typedef void DSPModuleCleanupFunctionPrototype(void);
typedef DSPModuleReturnCode DSPModuleStateChangedFunctionPrototype(const DSPLoaderAPI *, DSPState prevstate, DSPState currstate);

DSPMODULE_API extern const unsigned short dspmodule_requiredAPIversion;
// [dspmodule_startup]: 'dispname' is not required to be set.
DSPMODULE_API DSPModuleStartupFunctionPrototype dspmodule_startup;
DSPMODULE_API DSPModuleProcessFunctionPrototype dspmodule_process;
DSPMODULE_API DSPModuleCleanupFunctionPrototype dspmodule_cleanup;
DSPMODULE_API DSPModuleStateChangedFunctionPrototype dspmodule_statechanged;

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

static inline float absf(float in) { return in < 0 ? -in : in; }

static inline float adjf(float in, float adj) // adjunctf.
{
    if (!adj) return in;
    if (adj < 0 && absf(in) < absf(adj)) return 0;
    return in + adj * signf(in);
}

static inline float lerpf(float from, float to, float factor)
{ return from + (to - from) * clampf(factor, 0, 1); }

#ifdef __cplusplus
}
#endif

#endif
