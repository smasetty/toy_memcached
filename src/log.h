#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <ctime>
#include <stdarg.h>
#include <stdlib.h>

#define LOG_HIGH  0x0
#define LOG_DEBUG  0x1
#define LOG_LOW  0x2
#define LOG_VERBOSE  0x4

class Logger {
public:
    void SetOuputStream(std::ostream *out) {
        if (!out) {
            this->out = &std::cout;
            return;
        }
        this->out = out;
    }

    explicit Logger() {
        /*
         * Log stuff out to terminal, in-case the user forgets
         * to specify an output stream
         */
        this->out = &std::cout;
    }

    const char* LogLevelToString(int logLevel) {
        switch(logLevel) {
            case LOG_HIGH:
                return "HIGH";
            case LOG_LOW:
                return "LOW";
            case LOG_DEBUG:
                return "DEBUG";
            case LOG_VERBOSE:
                return "VERBOSE";
        }
        return "UNKNOWN";
    }

	std::ostream& GetLogInstance(int LogLevel);

private:
	std::ostream* out;
	/*
	 * Block any copy operations
	 */
	Logger(const Logger&);
	Logger operator=(const Logger&);
};

void InitLogging(int logLevel, std::ostream* out);
void LogMsg(int logLevel, const char* format, ...);

#endif /* LOG_H */
