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
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif
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
#include <memory>
#include "fmt/format.h"
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <condition_variable>
#include <mutex>
#include "nlohmann/detail/exceptions.hpp"
#include <istream>
extern const char *build_date;
std::condition_variable fastlogin_cv;
std::mutex fastlogin_mtx;
bool fastloginfinshed{ false };
#include "FastLoginListener.hpp"
#include <system_error>

// Credentials are read from the MEGA_EMAIL and MEGA_PWD environment variables.
// Get your APP_KEY for free at https://mega.io/developers#source-code
constexpr const char APP_KEY[] = "9gETCbhB";
constexpr const char USER_AGENT[] = "Simple-Client example app";

namespace {
bool getTimestampForFile(char *filetimpestamp) {
	struct tm aTime;
	time_t theTime;
	time(&theTime);
#ifdef _WIN32
	localtime_s(&aTime, &theTime);
#else
	localtime_r(&theTime, &aTime);
#endif
	int month = aTime.tm_mon + 1;
	int year = aTime.tm_year + 1900;
	std::snprintf(filetimpestamp, 18, "%d-%02d-%02d-%02d%02d%02d", year, month, aTime.tm_mday, aTime.tm_hour,
				  aTime.tm_min, aTime.tm_sec);
	return true;
}

void write_session_key_to_disk(const char *session_key, const std::string &path) {
	std::ofstream out(path);
	out.write(session_key, strlen(session_key));
	out.close();
}

std::string read_session_key_from_disk(const std::string &path) {
	std::ifstream input(path);
	char buffer[64];
	std::string key;
	while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0) {
		key += std::string(buffer, input.gcount());
	}
	input.close();
	return key;
}
}; // namespace

int main(int argc, char *argv[]) {
	fmt::println("Gabriel's Mega Client");
	fmt::println("Compiled With: {}", COMPILED_WITH);
	fmt::println("Git: {} {}", GIT_REV, GIT_BRANCH);
	fmt::println("Build date: {}", build_date);

	constexpr option ceasa_testing_options[] = {
		{ "files", required_argument, nullptr, 'f' },	 { "directory", required_argument, nullptr, 'd' },
		{ "sufix", required_argument, nullptr, 's' },	 { "remote_folder", required_argument, nullptr, 'r' },
		{ "log_file", required_argument, nullptr, 'l' }, { nullptr, 0 }
	};
	int opt{};
	int longindex{};
	std::string directory;
	std::string sufix;
	std::string remote_folder;
	std::string log_file;
	std::vector<std::string> files;
	while ((opt = getopt_long(argc, argv, "f:d:s:r:", ceasa_testing_options, &longindex)) != -1) {
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
			case 'l': log_file = std::string(optarg); break;
			case 'r': remote_folder = std::string(optarg); break;
			default: break;
		}
	}
	if (remote_folder.empty()) {
		remote_folder = "MEGA_uploads";
	}

	std::ifstream config_file_handler(gmegauploadconfig::gmegaupload_config_filename);
	if (!config_file_handler.good()) {
		fmt::println("Could not open {}", gmegauploadconfig::gmegaupload_config_filename);
		fmt::println("Create a config file {} like this:\n{}", gmegauploadconfig::gmegaupload_config_filename,
					 gmegauploadconfig::create_config_example());
		return 1;
	}
	nlohmann::json data;
	gmegauploadconfig::config conf;
	try {
		data = nlohmann::json::parse(config_file_handler);
		config_file_handler.close();
		conf = data.get<gmegauploadconfig::config>();

	} catch (nlohmann::detail::out_of_range &e) {
		fmt::println("problema extraindo o json de {}: {}", gmegauploadconfig::gmegaupload_config_filename, e.what());
		return 1;
	}

	// Route SDK logs to a file. Override the path with MEGA_LOG_FILE.

	const std::string logFilePath = log_file.empty() ? "" : log_file;

	// Tee logs to stdout by default. Disable with MEGA_LOG_STDOUT=0 (or =false).
	const char *envStdout = "true"; // colocar "false" para desligar
	const bool logToStdout =
		!envStdout || *envStdout == '\0' || (envStdout[0] != '0' && envStdout[0] != 'f' && envStdout[0] != 'F');

	char timestamp[18];
	getTimestampForFile(timestamp);
	std::string new_sufix(sufix);
	new_sufix += "_";
	new_sufix += timestamp;

	const char *envEmail = conf.get_mega_email();
	const char *envPassword = conf.get_mega_pwd();
	if (!envEmail || !*envEmail || !envPassword || !*envPassword) {
		std::cerr << "Set the MEGA_EMAIL and MEGA_PWD environment variables before running." << std::endl;
		return 1;
	}
#ifdef _WIN32
	HANDLE h = CreateMutexA(nullptr, FALSE, "Global\\GMegaUploadMutex");
	WaitForSingleObject(h, INFINITE);
#else
	int fd = open("/tmp/gmegauploadapp.lock", O_CREAT | O_RDWR, 0644);
	flock(fd, LOCK_EX);
#endif
	/* Começando o processamento do MEGA API */
	static SimpleClientLogger fileLogger(logFilePath, logToStdout);
	mega::MegaApi::addLoggerObject(&fileLogger, /*singleExclusiveLogger=*/true);
	mega::MegaApi::setLogLevel(mega::MegaApi::LOG_LEVEL_INFO);

	// fileLogger.log(nullptr, mega::MegaApi::LOG_LEVEL_INFO, __FUNCTION__,
	// 			   fmt::format("mega_email {}; mega_pwd {}", envEmail, envPassword).c_str());

	const std::string sevenzip_path = std::string(conf.get_sevenzip_path());
	std::pair<int, std::unique_ptr<std::string>> zip_result =
		call_zip_app(&sevenzip_path, &directory, &new_sufix, &files);

	if (zip_result.first != 0) {
		fileLogger.log(nullptr, mega::MegaApi::LOG_LEVEL_ERROR, __FUNCTION__,
					   fmt::format("Failed to create zip file temp directory: {}", *zip_result.second).c_str());
		return 1;
	}
	// Check the documentation of MegaApi to know how to enable local caching
	mega::MegaApi *megaApi = new mega::MegaApi(APP_KEY, ".", USER_AGENT);

	std::filesystem::path mypath = std::filesystem::path(*zip_result.second);

	GListener listener(&mypath, remote_folder);

	FastLoginListener fl;
	{
		std::string session_key = read_session_key_from_disk(conf.get_session_key_file());
		megaApi->fastLogin(session_key.c_str(), &fl);
		std::unique_lock<std::mutex> lock(fastlogin_mtx);
		fastlogin_cv.wait(lock, [] { return fastloginfinshed; });
	}
	megaApi->removeRequestListener(&fl);
	megaApi->addListener(&listener);
	if (fl.isLoginSuccessful()) {
		megaApi->fetchNodes();
	} else {
		megaApi->login(envEmail, envPassword);
	}
	// Listener to receive information about all request and transfers
	// It is also possible to register a different listener per request/transfer
	// Login. You can get the result in the onRequestFinish callback of your listener

	// You can use the main thread to show a GUI or anything else. MegaApi runs in a background
	// thread.
	while (!listener.finished) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	std::error_code errorCode;
	if (!std::filesystem::remove(mypath, errorCode)) {
		fileLogger.log(nullptr, mega::MegaApi::LOG_LEVEL_ERROR, __FUNCTION__, errorCode.message().c_str());
	}
	// Aqui já é seguro deletar o diretório onde estavam os arquivos zipados

	// Add code here to exercise MegaApi
	char *sessionKey = megaApi->dumpSession();

	write_session_key_to_disk(sessionKey, conf.get_session_key_file());

	// LogoutListener logoutListener;
	megaApi->localLogout();
	// megaApi->logout(true, &logoutListener);
	megaApi->removeListener(&listener);
	delete megaApi;
	mega::MegaApi::removeLoggerObject(&fileLogger);
	delete sessionKey;
#ifdef _WIN32
	ReleaseMutex(h);
	CloseHandle(h);
#else
	flock(fd, LOCK_UN);
	close(fd);
#endif
	return 0;
}
