#include <fmt/core.h>
#include <memory>
#include <vector>
#include "compiled_with.h"
#include "git_hash.h"
#include "thread_test.hpp"

extern const char *build_date;

std::unique_ptr<std::vector<std::vector<int>>> create_vector() {
	auto temp_vec = std::vector<int>();

	temp_vec.push_back(1);
	temp_vec.push_back(10);
	temp_vec.push_back(100);

	auto temp_vec_2 = std::vector<int>();

	temp_vec_2.push_back(2);
	temp_vec_2.push_back(20);
	temp_vec_2.push_back(200);

	auto ret_val = std::make_unique<std::vector<std::vector<int>>>();
	ret_val->push_back(std::move(temp_vec_2));
	ret_val->push_back(std::move(temp_vec));

	return ret_val;
}

int main() {
	fmt::println("Hello, World!");
	fmt::println("Compiled With: {}", COMPILED_WITH);
	fmt::println("Git: {} {}", GIT_REV, GIT_BRANCH);
	fmt::println("Build date: {}", build_date);
	std::unique_ptr<std::vector<std::vector<int>>> v = create_vector();
	for (auto const &i : *v) {
		for (auto &j : i) {
			fmt::println("--> {}", j);
		}
		fmt::println("======");
	}
	fmt::println("");
	while (!v->empty()) {
		auto i = v->back();
		for (auto j : i) fmt::println("--> {}", j);
		v->pop_back();
		fmt::println("======");
	}

	fmt::println("========== início threads ==========");
	thread_main();
	return 0;
}
