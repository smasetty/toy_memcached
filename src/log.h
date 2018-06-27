#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <ctime>

enum LogLevel {
	LOG_HIGH = 0x0,
	LOG_DEBUG = 0x1,
	LOG_LOW = 0x2,
	LOG_VERBOSE = 0x4
};

class Logger {
public:
	void SetOuputStream(std::ostream *out) {
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

std::ostream& Logger::GetLogInstance(int LogLevel)
{
		std::ostream& outputStream = *(this->out);
		std::time_t timeStamp = time(0);
		std::string timeStampString(std::ctime(&timeStamp));

		timeStampString.pop_back();
		outputStream << timeStampString << " - " << LogLevelToString(LogLevel) << " - ";
		return *(this->out);
}

#endif /* LOG_H */
