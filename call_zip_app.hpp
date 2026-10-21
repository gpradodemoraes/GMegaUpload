#pragma once
#ifndef _CALL_ZIP_APP_HPP_
#define _CALL_ZIP_APP_HPP_
#include <string>
#include <vector>
#include <utility>
#include <memory>

std::pair<int, std::unique_ptr<std::string>> call_zip_app(const std::string *zip_executable,
														  const std::string *root_path, const std::string *sufix,
														  const std::vector<std::string> *file_list, size_t split_size);
#endif
