#include "SimpleClientLogger.hpp"
#include <iostream>
#include <chrono>
#include <string>
#include <mega/log_level.h>

SimpleClientLogger::SimpleClientLogger(const std::string &path, bool logToStdout)
	: mStream(path, std::ios::app), mLogToStdout(logToStdout) {
	if (!mStream) {
		std::cerr << "SimpleClientLogger: failed to open " << path << std::endl;
	}
}

void SimpleClientLogger::log(const char * /*time*/, int loglevel, const char *source, const char *message
#ifdef ENABLE_LOG_PERFORMANCE
							 ,
							 const char **directMessages = nullptr, size_t *directMessagesSizes = nullptr,
							 int numberMessages = 0
#endif
) {
	std::lock_guard<std::mutex> lk(mMutex);

	// Build HH:MM:SS.mmm timestamp ourselves to match MegaCLILogger's format.
	using namespace std::chrono;
	const auto now = system_clock::now();
	const auto t = system_clock::to_time_t(now);
	const auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
	char hms[16];
	std::strftime(hms, sizeof(hms), "%H:%M:%S", std::localtime(&t));
	char timestamp[32];
	std::snprintf(timestamp, sizeof(timestamp), "%s.%03lld", hms, static_cast<long long>(ms));

	// Left-pad the level name to 7 chars (longest is "WARNING"/"VERBOSE")
	// so log columns line up.
	char level[8];
	std::snprintf(level, sizeof(level), "%-7s", mega::toString(static_cast<mega::LogLevel>(loglevel)));

	std::string s;
	s.reserve(1024);
	s += '[';
	s += timestamp;
	s += "] ";
	s += level;
	s += " | ";
	if (message) s += message;
#ifdef ENABLE_LOG_PERFORMANCE
	for (int i = 0; i < numberMessages; ++i) {
		s.append(directMessages[i], directMessagesSizes[i]);
	}
#endif
	if (source && *source != '\0') {
		s += " [";
		s += source;
		s += "]";
	}
	s += '\n';

	auto write = [&](std::ostream &os) { os << s; };

	if (mStream) {
		write(mStream);
		mStream.flush();
	}
	if (mLogToStdout) {
		write(std::cout);
	}
}
