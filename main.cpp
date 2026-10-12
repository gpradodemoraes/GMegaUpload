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

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <megaapi.h>
#include <string>
#include <thread>
#include <time.h>
#include "SimpleClientLogger.hpp"
#include "GListener.hpp"
#include <fmt/core.h>
#include "git_hash.h"
#include "compiled_with.h"
extern const char *build_date;

// Credentials are read from the MEGA_EMAIL and MEGA_PWD environment variables.
// Get your APP_KEY for free at https://mega.io/developers#source-code
constexpr auto APP_KEY = "9gETCbhB";
constexpr auto USER_AGENT = "Simple-Client example app";

static bool readYes() {
	char c;
	std::cin >> c; // skips whitespace, waits for Enter
	return (c == 'y' || c == 'Y');
}

std::string displayTime(time_t t) {
	char timebuf[32];
	strftime(timebuf, sizeof timebuf, "%c", localtime(&t));
	return timebuf;
}

int main() {
	fmt::println("Gabriel's Mega Client");
	fmt::println("Compiled With: {}", COMPILED_WITH);
	fmt::println("Git: {} {}", GIT_REV, GIT_BRANCH);
	fmt::println("Build date: {}", build_date);
	// Route SDK logs to a file. Override the path with MEGA_LOG_FILE.
	const char *envLogPath = std::getenv("MEGA_LOG_FILE");
	const std::string logFilePath = (envLogPath && *envLogPath) ? envLogPath : "simple_client.log";

	// Tee logs to stdout by default. Disable with MEGA_LOG_STDOUT=0 (or =false).
	const char *envStdout = std::getenv("MEGA_LOG_STDOUT");
	const bool logToStdout =
		!envStdout || *envStdout == '\0' || (envStdout[0] != '0' && envStdout[0] != 'f' && envStdout[0] != 'F');

	static SimpleClientLogger fileLogger(logFilePath, logToStdout);
	mega::MegaApi::addLoggerObject(&fileLogger, /*singleExclusiveLogger=*/true);
	mega::MegaApi::setLogLevel(mega::MegaApi::LOG_LEVEL_INFO);

	// Check the documentation of MegaApi to know how to enable local caching
	mega::MegaApi *megaApi = new mega::MegaApi(APP_KEY, ".", USER_AGENT);

	GListener listener;

	// Listener to receive information about all request and transfers
	// It is also possible to register a different listener per request/transfer
	megaApi->addListener(&listener);

	const char *envEmail = std::getenv("MEGA_EMAIL");
	const char *envPassword = std::getenv("MEGA_PWD");
	if (!envEmail || !*envEmail || !envPassword || !*envPassword) {
		std::cerr << "Set the MEGA_EMAIL and MEGA_PWD environment variables before running." << std::endl;
		return 1;
	}

	// Login. You can get the result in the onRequestFinish callback of your listener
	megaApi->login(envEmail, envPassword);

	// You can use the main thread to show a GUI or anything else. MegaApi runs in a background
	// thread.
	while (!listener.finished) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	// Add code here to exercise MegaApi

#ifdef HAVE_LIBUV
	std::cout << "Do you want to enable the local HTTP server (y/n enter)?" << std::endl;
	if (readYes()) {
		megaApi->httpServerStart();
		megaApi->httpServerSetRestrictedMode(MegaApi::HTTP_SERVER_ALLOW_ALL);
		megaApi->httpServerEnableFileServer(true);
		megaApi->httpServerEnableFolderServer(true);
		std::cout << "You can browse your account now! http://127.0.0.1:4443/" << std::endl;
	}
#endif

	std::cout << "Press y enter to exit the app..." << std::endl;
	while (!readYes()) {
	}

#ifdef HAVE_LIBUV
	megaApi->httpServerStop();
#endif

	megaApi->removeListener(&listener);
	delete megaApi;
	mega::MegaApi::removeLoggerObject(&fileLogger);
	return 0;
}
