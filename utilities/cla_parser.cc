#include "cla_parser.h"

#include <map>
#include <string>

ClaParser::ClaParser(int argc, char* argv[]) {
	valid_args_ = true;

	for (int i = 1; i < argc; i += 2) {
		if (argv[i][0] != '-' || argv[i+1] == nullptr) {
		valid_args_ = false;
		return;
    } else {
		option_to_arg_[ std::string(argv[i]) ] = std::string(argv[i+1]);
    }
  }
}

std::string ClaParser::get_argument(std::string option) {
	std::map<std::string, std::string>::const_iterator entry;
	if ((entry = option_to_arg_.find(option)) != option_to_arg_.end()) {
		return entry->second;
	}

	static std::string empty_string("");
	return empty_string;
}
