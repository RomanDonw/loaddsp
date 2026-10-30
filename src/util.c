#include "util.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

static void _log(const char *location, DSPLogLevel level, const char *format, va_list args)
{
    const char *levelstr;
    switch (level)
    {
        case DSPLogLevel_Debug:
            levelstr = "DEBUG";
            break;
        
        case DSPLogLevel_Info:
            levelstr = "INFO";
            break;

        case DSPLogLevel_Warning:
            levelstr = "WARN";
            break;

        case DSPLogLevel_Error:
            levelstr = "ERROR";
            break;

        case DSPLogLevel_Fatal:
            levelstr = "FATAL";
            break;

        default:
            return;
    }
    
    time_t now = time(NULL);
    char datebuff[128];
    strftime(datebuff, sizeof(datebuff), "%H:%M:%S", localtime(&now));
    printf("[%s %s %-5s] ", datebuff, location, levelstr);

    size_t count = 0;
    size_t formatsize = 1;
    for (const char *s = format; *s; s++) { formatsize++; if (*s == '\n') count++; }
    size_t fmtsize = formatsize + 24 * count;
    char fmt[fmtsize];
    {
        size_t i = 0;
        for (const char *s = format; *s; s++)
        {
            if (*s == '\n') { memcpy(fmt + i, "\n                      | ", 25); i += 25; }
            else fmt[i++] = *s;
        }
        fmt[i] = '\0';
    }
    
    vprintf(fmt, args);
    putchar('\n');
}

void loader_log(DSPLogLevel level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    _log("LOADER", level, format, args);
    va_end(args);
}

void module_log(DSPLogLevel level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    _log("MODULE", level, format, args);
    va_end(args);
}