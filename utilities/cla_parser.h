#ifndef CLA_PARSER_H_
#define CLA_PARSER_H_

#include <map>
#include <string>

class ClaParser {
  public:
	ClaParser(int argc, char* argv[]);

	// Returns true if the provided arguments are correct, false otherwise.
	// Arguments are correct if they conform to the syntax ( -X <option> )*.

	bool valid_args() { return valid_args_; }

	// Returns the argument corresponding to 'option' (or "", if not provided).
	std::string get_argument(std::string option);

  private:
	bool valid_args_;
	std::map<std::string, std::string> option_to_arg_;
};

#endif // CLA_PARSER_H_
