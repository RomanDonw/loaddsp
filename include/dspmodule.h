#ifndef DSPMODULE_H
#define DSPMODULE_H

#include <getopt.h>

#define DSPMODULE_API __attribute__((visibility("default")))

typedef unsigned short DSPModuleStartupFunctionPrototype(const char **name, unsigned short *inportscount, unsigned short *outportscount, int argc, char * const argv[]);
typedef unsigned short DSPModuleProcessFunctionPrototype(const float * const inbuffers[], float * const outbuffers[], unsigned long samplescount);
typedef void DSPModuleCleanupFunctionPrototype(void);

DSPMODULE_API DSPModuleStartupFunctionPrototype dspmodule_startup;
DSPMODULE_API DSPModuleProcessFunctionPrototype dspmodule_process;
DSPMODULE_API DSPModuleCleanupFunctionPrototype dspmodule_cleanup;

#endif