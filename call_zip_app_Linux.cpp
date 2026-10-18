#include <filesystem>
#include <fmt/format.h>
#include <string>
#include <utility>
#include <memory>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <fmt/base.h>

std::pair<int, std::unique_ptr<std::string>> call_zip_app(const std::string *zip_executable,
														  const std::string *root_path, const std::string *sufix,
														  const std::vector<std::string> *file_list) {
	std::filesystem::path temp_path = std::filesystem::temp_directory_path() / *sufix;
	while (std::filesystem::exists(temp_path)) {
		temp_path /= "x";
	}

	if (!std::filesystem::create_directories(temp_path)) {
		return { 1, std::make_unique<std::string>(
						fmt::format("Failed to create temporary directory: {}", temp_path.string())) };
	}

	std::filesystem::path zip_path = temp_path / (*sufix + ".7z");
	std::string zip_file = { zip_path.u8string() };
	std::vector<std::string> params;
	params.push_back(*zip_executable);
	params.emplace_back("a");
	params.emplace_back("-v10m");
	params.push_back(zip_file);

	for (const auto &file : *file_list) {
		params.push_back(file);
	}

	std::vector<char *> myargs;
	for (auto &s : params) {
		fmt::println("FMT: {}", s);
		myargs.push_back(s.data());
	}
	myargs.push_back(nullptr);

	pid_t pid = fork();
	if (pid < 0) {
		std::perror("fork");
		return { 1, std::make_unique<std::string>("process failed to fork!") };
	}
	if (pid == 0) {
		if (chdir(root_path->c_str()) != 0) {
			std::perror("chdir");
			_exit(1);
		}
		// Child process: replace itself with 7z
		execvp(myargs[0], myargs.data());

		// Only reached if execvp failed (e.g. 7z not installed)
		std::fprintf(stderr, "execvp failed: %s\n", std::strerror(errno));
		_exit(127);
	}

	// Parent process: wait for the child to finish
	int status = 0;
	if (waitpid(pid, &status, 0) < 0) {
		std::perror("waitpid");
		return { 0, std::make_unique<std::string>("there was an error with the zip child process") };
	}

	return { 0, std::make_unique<std::string>(temp_path.string()) };
}
