#pragma once
#ifndef _SPDLOGMEGALOGGER_HPP_
#define _SPDLOGMEGALOGGER_HPP_
#include <memory>
#include <megaapi.h>
#include <spdlog/logger.h>
#include <utility>
#include <spdlog/common.h>
#include <string>
#include <charconv>

class SpdlogMegaLogger : public mega::MegaLogger {
   public:
	explicit SpdlogMegaLogger(std::shared_ptr<spdlog::logger> logger) : mLogger(std::move(logger)) {}

	void log([[maybe_unused]] const char *time, int loglevel, const char *source, const char *message
#ifdef ENABLE_LOG_PERFORMANCE
			 ,
			 const char **directMessages = nullptr, size_t *directMessagesSizes = nullptr, unsigned numberMessages = 0
#endif
			 ) override {
		if (!mLogger) return;

		const auto level = toSpdlogLevel(loglevel);
		if (!mLogger->should_log(level)) return;

		std::string src(source);
		const auto pos = src.rfind(':');
		if (pos == std::string::npos)
			mLogger->log(spdlog::source_loc{ source, 0, nullptr }, level, "{}", message);
		else {
			int line = 0;
			const auto num = src.substr(pos + 1);
			const auto [ptr, ec] = std::from_chars(num.data(), num.data() + num.size(), line);

			if (ec != std::errc{} || ptr != num.data() + num.size())
				mLogger->log(spdlog::source_loc{ source, 0, nullptr }, level, "{}", message);
			else
				mLogger->log(spdlog::source_loc{ src.substr(0, pos).c_str(), line, nullptr }, level, "{}", message);
		}

#ifdef ENABLE_LOG_PERFORMANCE
		// If you build the SDK with ENABLE_LOG_PERFORMANCE, parts of the message
		// arrive via directMessages/directMessagesSizes instead of `message`.
		// Concatenate them here if you need them.
#endif
	}

   private:
	static spdlog::level::level_enum toSpdlogLevel(int megaLevel) {
		switch (megaLevel) {
			case mega::MegaApi::LOG_LEVEL_FATAL: return spdlog::level::critical;
			case mega::MegaApi::LOG_LEVEL_ERROR: return spdlog::level::err;
			case mega::MegaApi::LOG_LEVEL_WARNING: return spdlog::level::warn;
			case mega::MegaApi::LOG_LEVEL_INFO: return spdlog::level::info;
			case mega::MegaApi::LOG_LEVEL_DEBUG: return spdlog::level::debug;
			case mega::MegaApi::LOG_LEVEL_MAX: return spdlog::level::trace;
			default: return spdlog::level::info;
		}
	}

	std::shared_ptr<spdlog::logger> mLogger;
};
#endif
