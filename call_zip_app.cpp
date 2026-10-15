#include <Windows.h>
#include <filesystem>
#include <fmt/format.h>
#include <string>
#include <utility>
#include <memory>
#include <vector>

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
	std::string params = "a -v1024k ";
	params.append(zip_file);

	for (const auto &file : *file_list) {
		params += " \"" + file + "\"";
	}

	SHELLEXECUTEINFOA sei = { 0 };

	sei.cbSize = sizeof(SHELLEXECUTEINFOA);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS; // get a handle to the launched process
	sei.hwnd = NULL;
	sei.lpVerb = "open";				  // verb: "open", "edit", "print", "runas", etc.
	sei.lpFile = zip_executable->c_str(); // program/file to launch
	sei.lpParameters = params.c_str();	  // command-line args, if any
	sei.lpDirectory = root_path->c_str(); // working directory
	sei.nShow = SW_SHOWNORMAL;			  // how the window is shown
	sei.hInstApp = NULL;

	if (!ShellExecuteExA(&sei)) {
		DWORD err = GetLastError();
		return { 1, std::make_unique<std::string>(fmt::format("ShellExecuteExA failed, error code: {}", err)) };
	}

	// Since we used SEE_MASK_NOCLOSEPROCESS, we have a process handle we should close
	if (sei.hProcess) {
		// Optionally wait for the process to finish
		WaitForSingleObject(sei.hProcess, INFINITE);
		CloseHandle(sei.hProcess);
	}
	return { 0, std::make_unique<std::string>(temp_path.string()) };
}
