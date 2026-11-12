#include <filesystem>
#include <fmt/base.h>
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
#include <spdlog/spdlog.h>

std::pair<int, std::unique_ptr<std::string>> call_zip_app(const std::string *zip_executable,
														  const std::string *root_path, const std::string *sufix,
														  const std::vector<std::string> *file_list,
														  size_t split_size) {
	constexpr int BUFFERSIZE = 64;
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
	if (split_size > 0) params.emplace_back(fmt::format("-v{}b", split_size));
	params.push_back(zip_file);

	for (const auto &file : *file_list) {
		params.push_back(file);
	}

	std::vector<char *> myargs;
	for (auto &s : params) {
		SPDLOG_INFO("arg: {}", s);
		myargs.push_back(s.data());
	}
	int pipefd[2];
	if (pipe(pipefd) == -1) {
		std::perror("pipe");
		return { 1, std::make_unique<std::string>("Could not create PIPE!") };
	}
	myargs.push_back(nullptr);

	pid_t pid = fork();
	if (pid < 0) {
		std::perror("fork");
		return { 1, std::make_unique<std::string>("process failed to fork!") };
	}
	if (pid == 0) {
		// THIS IS THE CHILD !
		close(pipefd[0]);				// close unused read end
		dup2(pipefd[1], STDOUT_FILENO); // stdout -> pipe
		dup2(pipefd[1], STDERR_FILENO); // optional: stderr -> pipe
		close(pipefd[1]);				// fd already duplicated
		if (chdir(root_path->c_str()) != 0) {
			std::perror("chdir");
			_exit(1);
		}
		// Child process: replace itself with 7z
		execvp(myargs[0], myargs.data());

		// Only reached if execvp failed (e.g. 7z not installed)
		SPDLOG_ERROR("execvp failed: {}", std::strerror(errno));
		_exit(127);
	}

	// Parent process: wait for the child to finish
	close(pipefd[1]);
	std::string output;
	char buf[BUFFERSIZE];
	ssize_t n;
	while ((n = read(pipefd[0], buf, sizeof(buf))) != 0) {
		if (n == -1) {
			if (errno == EINTR) continue;
			perror("read");
			break;
		}
		output.append(buf, n);
		int start = 0;
		int end = 0;
		while ((end = output.find("\n", start)) != std::string::npos) {
			SPDLOG_INFO("7zip:{}", output.substr(start, end - start));
			start = end + 1;
		}
		if (start >= output.length())
			output = "";
		else
			output = output.substr(start);
	}
	close(pipefd[0]);

	int status = 0;
	if (waitpid(pid, &status, 0) < 0) {
		std::perror("waitpid");
		return { 1, std::make_unique<std::string>("there was an error with the zip child process") };
	}
	if (WIFEXITED(status)) SPDLOG_INFO("Exit code: {}", WEXITSTATUS(status));

	return { 0, std::make_unique<std::string>(temp_path.string()) };
}
