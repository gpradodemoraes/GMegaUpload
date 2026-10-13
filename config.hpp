#pragma once
#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_

#include <string>
#include <nlohmann/json.hpp>
#include "nlohmann/detail/macro_scope.hpp"

namespace gmegauploadconfig {
class config {
   private:
	std::string mega_email;
	std::string mega_pwd;

   public:
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(config, mega_email, mega_pwd)

	char *get_mega_email() { return (char *)mega_email.c_str(); }

	char *get_mega_pwd() { return (char *)mega_pwd.c_str(); }
};

// inline constexpr const char *identity_url = "https://identity.api.rackspacecloud.com/v2.0/tokens";
//
// inline constexpr const char *authenticate_json_char =
//         R"({"auth":{"RAX-KSKEY:apiKeyCredentials":{"username":"yourUserName","apiKey":"yourApiKey"}}})";

// inline constexpr const char *gmegaupload_access_filename = "gmegaupload_access.json";
inline constexpr const char *gmegaupload_config_filename = "gmegaupload_config.json";

std::string create_config_example() {
	nlohmann::json data;
	data["mega_email"] = "your_email@example.com";
	data["mega_pwd"] = "your_password";
	return data.dump(4);
}

// struct cloudfiles_info {
//         std::string region;
//         std::string public_url;
//         std::string access_token;
// };
} // namespace gmegauploadconfig
#endif
