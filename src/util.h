#ifndef UTIL_H
#define UTIL_H

#include "dspmodule.h"

extern DSPLogLevel currloglevel;

void loader_log(DSPLogLevel level, const char *format, ...);
void module_log(DSPLogLevel level, const char *format, ...);

#endif