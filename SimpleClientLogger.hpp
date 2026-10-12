#include <megaapi.h>
#include <ios>
#include <mutex>
#include <fstream>
#include <string>

class SimpleClientLogger : public mega::MegaLogger {
   public:
	SimpleClientLogger(const std::string &path, bool logToStdout);

	void log(const char * /*time*/, int loglevel, const char *source, const char *message
#ifdef ENABLE_LOG_PERFORMANCE
			 ,
			 const char **directMessages = nullptr, size_t *directMessagesSizes = nullptr, int numberMessages = 0
#endif
			 ) override;
   private:
	std::ofstream mStream;
	bool mLogToStdout;
	std::mutex mMutex;
};
