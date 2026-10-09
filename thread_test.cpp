#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <random>

std::mutex mtx;
std::condition_variable cv;
std::queue<std::string> dataQueue;
bool finished = false;

static void producer() {
	std::vector<std::string> arquivos = {

		".clang-format",
		".editorconfig",
		".gitattributes",
		".gitignore",
		"CMakeLists.txt",
		"CMakePresets.json",
		"compile_commands.json",
		"compiled_with.h",
		"git_hash.h.in",
		"LICENSE",
		"main.cpp",
		"README.md",
		"thread_test.cpp",
		"thread_test.hpp",
		"vcpkg-configuration.json",
		"vcpkg.json",
		"version.cmake"
	};
	for (const auto &file : arquivos) {
		{
			std::lock_guard<std::mutex> lock(mtx);
			dataQueue.push(file);
			std::cout << "Produced: " << file << "\n";
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		cv.notify_one(); // Wake up consumer
	}

	{
		std::lock_guard<std::mutex> lock(mtx);
		finished = true;
	}
	cv.notify_one();
}

static void process_file(std::string filename, int randomNum) {
	std::cout << "START FILE: " << filename << "\n";

	std::this_thread::sleep_for(std::chrono::milliseconds(randomNum));
	std::cout << "FINISHES FILE (" << randomNum << "): " << filename << "\n";
}

int thread_main() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(1, 5000);

	std::thread prod(producer);

	std::vector<std::thread> consumers_list;

	while (true) {
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [] { return !dataQueue.empty() || finished; });

		while (!dataQueue.empty()) {
			std::string value = dataQueue.front();
			dataQueue.pop();
			consumers_list.emplace_back(process_file, value, dist(gen));
		}

		if (finished && dataQueue.empty()) {
			for (auto &c : consumers_list) c.join();
			break;
		}
	}
	prod.join();

	return 0;
}
