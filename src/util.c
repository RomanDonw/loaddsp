#include "util.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include <spa/utils/ansi.h>

DSPLogLevel currloglevel = DSPLogLevel_Info;

static void _log(const char *location, DSPLogLevel level, const char *format, va_list args)
{
    if (level < currloglevel) return;

    const char *levelstr, *colorstr;
    switch (level)
    {
        case DSPLogLevel_Debug:
            levelstr = "DEBUG";
            colorstr = SPA_ANSI_BRIGHT_MAGENTA;
            break;

        case DSPLogLevel_Info:
            levelstr = "INFO";
            colorstr = SPA_ANSI_GREEN;
            break;

        case DSPLogLevel_Warning:
            levelstr = "WARN";
            colorstr = SPA_ANSI_BRIGHT_YELLOW;
            break;

        case DSPLogLevel_Error:
            levelstr = "ERROR";
            colorstr = SPA_ANSI_BRIGHT_RED;
            break;

        case DSPLogLevel_Fatal:
            levelstr = "FATAL";
            colorstr = SPA_ANSI_BOLD SPA_ANSI_DARK_RED;
            break;

        default:
            return;
    }

    time_t now = time(NULL);
    char datebuff[9];
    strftime(datebuff, sizeof(datebuff), "%H:%M:%S", localtime(&now));
    printf(SPA_ANSI_RESET SPA_ANSI_CYAN "[" SPA_ANSI_BRIGHT_BLACK "%s " SPA_ANSI_BOLD SPA_ANSI_BRIGHT_WHITE "%s " SPA_ANSI_RESET
        "%s%-5s" SPA_ANSI_RESET SPA_ANSI_CYAN "] " SPA_ANSI_RESET,
        datebuff, location, colorstr, levelstr);

    size_t count = 0;
    size_t formatsize = 1;
    for (const char *s = format; *s; s++) { formatsize++; if (*s == '\n') count++; }
    static const char tabstr[] = "                      " SPA_ANSI_RESET SPA_ANSI_CYAN "| " SPA_ANSI_RESET;
    size_t fmtsize = formatsize + (sizeof(tabstr) - 1) * count;
    char fmt[fmtsize];
    {
        size_t i = 0;
        for (const char *s = format; *s; s++)
        {
            fmt[i++] = *s;
            if (*s == '\n') { memcpy(fmt + i, tabstr, sizeof(tabstr) - 1); i += sizeof(tabstr) - 1; }
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