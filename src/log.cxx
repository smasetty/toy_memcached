#include <iostream>
#include <ctime>
#include <stdarg.h>
#include <stdlib.h>
#include "log.h"

/*
 * By default print all messages
 */
static int configuredLogLevel = LOG_HIGH;
#define MAX_LOG_SIZE 256
Logger log;

/*
 * A simple output stream wrapper which adds the LogLevel in string format
 * and a human readable timestamp
 */
std::ostream& Logger::GetLogInstance(int LogLevel)
{
    std::ostream& outputStream = *(this->out);
    std::time_t timeStamp = time(0);
    std::string timeStampString(std::ctime(&timeStamp));

    timeStampString.pop_back();
    outputStream << timeStampString << " - " << LogLevelToString(LogLevel) << " - ";
    return *(this->out);
}

static void SetLogLevel(int logLevel)
{
    configuredLogLevel = logLevel;
}

void InitLogging(int logLevel, std::ostream* out)
{
    SetLogLevel(logLevel);
    log.SetOuputStream(out);
}

void LogMsg(int logLevel, const char* format, ...)
{
    if (logLevel > configuredLogLevel)
        return;

    char buffer[MAX_LOG_SIZE];
    va_list argptr;

    va_start(argptr, format);
    vsnprintf(buffer, MAX_LOG_SIZE, format, argptr);
    va_end(argptr);

    log.GetLogInstance(logLevel) << buffer << std::endl;
}
