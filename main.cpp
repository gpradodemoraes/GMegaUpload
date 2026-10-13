/**
 * @file examples/simple_client/simple_client.cpp
 * @brief Example app
 *
 * (c) 2013-2025 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of the MEGA SDK - Client Access Engine.
 *
 * Applications using the MEGA API must present a valid application key
 * and comply with the the rules set forth in the Terms of Service.
 *
 * The MEGA SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @copyright Simplified (2-clause) BSD License.
 *
 * You should have received a copy of the license along with this
 * program.
 */

#include <mega/log_level.h>
#include "getopt.h"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <megaapi.h>
#include <string>
#include <thread>
#include <string>
#include <vector>
#include "config.hpp"
#include "SimpleClientLogger.hpp"
#include "GListener.hpp"
#include <fmt/base.h>
#include "git_hash.h"
#include "compiled_with.h"
#include "call_zip_app.hpp"
#include <utility>

extern const char *build_date;

// Credentials are read from the MEGA_EMAIL and MEGA_PWD environment variables.
// Get your APP_KEY for free at https://mega.io/developers#source-code
constexpr const char APP_KEY[] = "9gETCbhB";
constexpr const char USER_AGENT[] = "Simple-Client example app";

namespace {
bool getTimestampForFile(char *filetimpestamp) {
	struct tm aTime;
	time_t theTime;
	time(&theTime);
	localtime_s(&aTime, &theTime);

	int month = aTime.tm_mon + 1;
	int year = aTime.tm_year + 1900;
	std::snprintf(filetimpestamp, 18, "%d-%02d-%02d-%02d%02d%02d", year, month, aTime.tm_mday, aTime.tm_hour,
				  aTime.tm_min, aTime.tm_sec);
	return true;
}
}; // namespace

int main(int argc, char *argv[]) {
	fmt::println("Gabriel's Mega Client");
	fmt::println("Compiled With: {}", COMPILED_WITH);
	fmt::println("Git: {} {}", GIT_REV, GIT_BRANCH);
	fmt::println("Build date: {}", build_date);

	constexpr option ceasa_testing_options[] = { { "files", required_argument, nullptr, 'f' },
												 { "directory", required_argument, nullptr, 'd' },
												 { "sufix", required_argument, nullptr, 's' },
												 { nullptr, 0 } };
	int opt{};
	int longindex{};
	std::string directory;
	std::string sufix;
	std::vector<std::string> files;
	while ((opt = getopt_long(argc, argv, "fd:s:", ceasa_testing_options, &longindex)) != -1) {
		switch (opt) {
			case 'd': directory = std::string(optarg); break;
			case 'f': {
				files.emplace_back(optarg);
				while (optind < argc && argv[optind][0] != '-') {
					files.emplace_back(argv[optind]);
					optind++;
				}
			} break;
			case 's': sufix = std::string(optarg); break;
			default: break;
		}
	}

	// Route SDK logs to a file. Override the path with MEGA_LOG_FILE.
	const char *envLogPath = std::getenv("MEGA_LOG_FILE");
	const std::string logFilePath = (envLogPath && *envLogPath) ? envLogPath : "simple_client.log";

	// Tee logs to stdout by default. Disable with MEGA_LOG_STDOUT=0 (or =false).
	const char *envStdout = std::getenv("MEGA_LOG_STDOUT");
	const bool logToStdout =
		!envStdout || *envStdout == '\0' || (envStdout[0] != '0' && envStdout[0] != 'f' && envStdout[0] != 'F');

	std::ifstream config_file_handler(gmegauploadconfig::gmegaupload_config_filename);
	if (!config_file_handler.good()) {
		fmt::println("Could not open {}", gmegauploadconfig::gmegaupload_config_filename);
		fmt::println("Create a config file {} like this:\n{}", gmegauploadconfig::gmegaupload_config_filename,
					 gmegauploadconfig::create_config_example());
		return 1;
	}
	nlohmann::json data = nlohmann::json::parse(config_file_handler);
	config_file_handler.close();
	gmegauploadconfig::config conf = data.get<gmegauploadconfig::config>();

	char timestamp[18];
	getTimestampForFile(timestamp);
	std::string new_sufix(sufix);
	new_sufix += "_";
	new_sufix += timestamp;

	std::pair<int, std::string> zip_result =
		call_zip_app(&std::string(conf.get_sevenzip_path()), &directory, &new_sufix, &files);

	if (zip_result.first != 0) {
		fmt::println("Failed to create zip file: {}", zip_result.second);
		return 1;
	}

	const char *envEmail = conf.get_mega_email();
	const char *envPassword = conf.get_mega_pwd();
	if (!envEmail || !*envEmail || !envPassword || !*envPassword) {
		std::cerr << "Set the MEGA_EMAIL and MEGA_PWD environment variables before running." << std::endl;
		return 1;
	}

	/* Começando o processamento do MEGA API */
	static SimpleClientLogger fileLogger(logFilePath, logToStdout);
	mega::MegaApi::addLoggerObject(&fileLogger, /*singleExclusiveLogger=*/true);
	mega::MegaApi::setLogLevel(mega::MegaApi::LOG_LEVEL_INFO);

	fileLogger.log(nullptr, mega::MegaApi::LOG_LEVEL_INFO, __FUNCTION__,
				   fmt::format("mega_email {}; mega_pwd {}", envEmail, envPassword).c_str());

	// Check the documentation of MegaApi to know how to enable local caching
	mega::MegaApi *megaApi = new mega::MegaApi(APP_KEY, ".", USER_AGENT);

	GListener listener;

	// Listener to receive information about all request and transfers
	// It is also possible to register a different listener per request/transfer
	megaApi->addListener(&listener);
	// Login. You can get the result in the onRequestFinish callback of your listener
	megaApi->login(envEmail, envPassword);

	// You can use the main thread to show a GUI or anything else. MegaApi runs in a background
	// thread.
	while (!listener.finished) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	// Add code here to exercise MegaApi

	megaApi->removeListener(&listener);
	delete megaApi;
	mega::MegaApi::removeLoggerObject(&fileLogger);
	return 0;
}
