#ifndef UTIL_H
#define UTIL_H

#include <getopt.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
    #define RESETGETOPT() \
        extern int optreset;\
        optind = 1;\
        optreset = 1;
#else
    #define RESETGETOPT() \
        optind = 0;
#endif

#endif