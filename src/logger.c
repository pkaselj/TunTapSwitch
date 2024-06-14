#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static void LOG_Raw_(const char* prefix, const char* format, va_list args)
{
    time_t rawtime;
    struct tm * timeinfo;
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    char timestamp[18] = {0};
    strftime(timestamp, 18, "%Y/%m/%d %H:%M:%S", timeinfo);

    printf("%s : [%s] ", timestamp, prefix);
    vprintf(format, args);
    printf("\n");
}

void LOG_Info(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LOG_Raw_("INF", format, args);
    va_end(args);
}

void LOG_Warn(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LOG_Raw_("WRN", format, args);
    va_end(args);
}

void LOG_Debug(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LOG_Raw_("DBG", format, args);
    va_end(args);
}

void LOG_Error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LOG_Raw_("ERR", format, args);
    va_end(args);
}
