#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include "call_zip_app.hpp"

void call_zip_app(const std::string *zip_executable, const std::string *root_path, const std::string *sufix,
				  const std::vector<std::string> *file_list) {
	//. "C:\Program Files\7-Zip\7z.exe" a -v10k megustamucho_outro.7z *.png

	std::string params = "a -v100k ";
	params += *root_path + "\\" + *sufix + ".7z";
	for (const auto &file : *file_list) {
		params += " " + file;
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
		printf("ShellExecuteExA failed, error code: %lu\n", err);
		return;
	}
	printf("Process launched successfully.\n");

	// Since we used SEE_MASK_NOCLOSEPROCESS, we have a process handle we should close
	if (sei.hProcess) {
		// Optionally wait for the process to finish
		WaitForSingleObject(sei.hProcess, INFINITE);
		CloseHandle(sei.hProcess);
	}
}
