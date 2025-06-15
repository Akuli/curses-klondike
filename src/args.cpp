/*
non-gnu-extension getopt doesn't support --long-arguments
so i did it myself, why not :D
doesn't support --, but nobody needs it for this program imo
*/

#include "args.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>
#include <iomanip>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

enum class OptionType { YESNO, INT };

struct OptionSpec {
	std::string_view name;
	std::optional<std::string_view> metavar;
	OptionType type;
	int min;
	int max;
	std::string_view desc;

	std::string get_name_with_metavar(char sep = ' ') const {
		if (this->metavar)
			return std::string(this->name) + sep + std::string(this->metavar.value());
		return std::string(this->name);
	}
};

static constexpr std::array<OptionSpec, 4> option_specs = {
	OptionSpec{ "--help", std::nullopt, OptionType::YESNO, 0, 0, "show this help message and exit" },
	OptionSpec{ "--no-colors", std::nullopt, OptionType::YESNO, 0, 0, "don't use colors, even if the terminal supports colors" },
	OptionSpec{ "--pick", "n", OptionType::INT, 1, 13*4 - (1+2+3+4+5+6+7), "pick n cards from stock at a time, default is 3" },
	OptionSpec{ "--discard-hide", std::nullopt, OptionType::YESNO, 0, 0, "only show topmost discarded card (not useful with --pick=1)" }
};
static constexpr int option_max_length = 14;

// represents an option and a value, if any
// needed because one arg token can come from 2 argv items: --pick 3
// or just 1: --no-colors, --pick=3
struct Token {
	OptionSpec spec;
	std::optional<std::string> value;
};

static bool is_valid_integer(const std::string& str, int min, int max)
{
	try {
		int n = std::stoi(str);
		return min <= n && n <= max;
	} catch(...) {
		return false;
	}
}

class Parser {
public:
	Parser(std::ostream& out, std::ostream& err, std::string argv0) : out(out), err(err), argv0(argv0) {}

	std::optional<Args> parse(std::deque<std::string>& args, int& status) const
	{
		bool tokenize_ok = true;
		std::vector<Token> tokens = {};
		while (!args.empty()) {
			std::optional<Token> token = this->get_token(args);
			if (!token) {
				tokenize_ok = false;
				break;
			}
			tokens.push_back(token.value());
		}

		if (!tokenize_ok || !this->check_duplicate_tokens(tokens)) {
			this->err << "Run '" << this->argv0 << " --help' for help." << std::endl;
			status = 2;
			return std::nullopt;
		}

		std::optional<Args> res = this->tokens_to_args(tokens);
		if (!res)
			status = 0;  // help printed (success)
		return res;
	}

private:
	std::ostream& out;
	std::ostream& err;
	std::string argv0;

	void print_help() const
	{
		this->out << "Usage: " << this->argv0;
		for (const OptionSpec& opt : option_specs)
			this->out << " [" << opt.get_name_with_metavar() << "]";

		this->out << "\n\nOptions:\n";
		for (const OptionSpec& opt : option_specs)
		{
			this->out << "  "
				<< std::left << std::setw(option_max_length) << opt.get_name_with_metavar()
				<< "  " << opt.desc << std::endl;
		}
	}

	std::optional<OptionSpec> find_from_option_specs(std::string name) const
	{
		auto name_matches = [&](const OptionSpec& spec){ return spec.name.find(name) == 0; };
		const OptionSpec *match = std::find_if(option_specs.begin(), option_specs.end(), name_matches);
		if (match == option_specs.end()) {
			this->err << this->argv0 << ": unknown option '" << name << "'" << std::endl;
			return std::nullopt;
		}

		const OptionSpec *match2 = std::find_if(match+1, option_specs.end(), name_matches);
		if (match2 != option_specs.end()) {
			this->err << this->argv0 << ":"
				<< " ambiguous option '" << name << "':"
				<< " could be '" << match->name << "'"
				<< " or '" << match2->name << "'" << std::endl;
			return std::nullopt;
		}

		return *match;
	}

	// argc and argv should point to remaining args, not always all the args
	std::optional<Token> get_token(std::deque<std::string>& arg_queue) const
	{
		std::string arg = arg_queue[0];
		arg_queue.pop_front();

		if (arg.length() < 3 || arg.find("--") != 0) {
			this->err << this->argv0 << ": unexpected argument: '" << arg << "'" << std::endl;
			return std::nullopt;
		}

		std::string name;
		std::optional<std::string> value;

		std::size_t i = arg.find("=");
		if (i != std::string::npos) {
			name = arg.substr(0, i);
			value = arg.substr(i+1);
		} else {
			name = arg;
			if (arg_queue.empty() || arg_queue[0].c_str()[0] == '-') {
				value = std::nullopt;
			} else {
				value = arg_queue[0];
				arg_queue.pop_front();
			}
		}

		std::optional<OptionSpec> spec = this->find_from_option_specs(name.c_str());
		if (!spec)
			return std::nullopt;

		switch(spec->type) {
		case OptionType::YESNO:
			if (value) {
				this->err << this->argv0 << ":"
					<< " use just '" << spec->name << "',"
					<< " not '" << spec->name << " something'"
					<< " or '" << spec->name << "=something'" << std::endl;
				return std::nullopt;
			}
			break;

		case OptionType::INT:
			if (!value) {
				this->err << this->argv0 << ": use"
					<< " '" << spec->get_name_with_metavar(' ') << "' or"
					<< " '" << spec->get_name_with_metavar('=') << "', not just"
					<< " '" << spec->name << "'" << std::endl;
				return std::nullopt;
			}
			if (!is_valid_integer(*value, spec->min, spec->max)) {
				this->err << this->argv0 << ":"
					<< " '" << spec->name << "' wants an integer"
					<< " between " << spec->min << " and " << spec->max << ","
					<< " not '" << *value << "'" << std::endl;
				return std::nullopt;
			}
			break;
		}

		return Token{ spec.value(), value };
	}

	bool check_duplicate_tokens(const std::vector<Token>& tokens) const
	{
		std::vector<std::string_view> names;
		for (const Token& token : tokens)
			names.push_back(token.spec.name);
		std::sort(names.begin(), names.end());

		auto duplicate = std::adjacent_find(names.begin(), names.end());
		if (duplicate != names.end()) {
			this->err << this->argv0 << ": repeated option '" << *duplicate << "'" << std::endl;
			return false;
		}
		return true;
	}

	std::optional<Args> tokens_to_args(const std::vector<Token>& tokens) const
	{
		Args result = {};
		for (Token token : tokens) {
			if (token.spec.name == "--help") {
				this->print_help();
				return std::nullopt;
			}

			if (token.spec.name == "--no-colors")
				result.color = false;
			else if (token.spec.name == "--pick")
				result.pick = std::stoi(token.value.value());  // already validated
			else if (token.spec.name == "--discard-hide")
				result.discardhide = true;
			else
				throw std::logic_error("unknown arg name: " + std::string(token.spec.name));
		}
		return result;
	}
};

std::optional<Args> parse_args(int& status, const std::vector<std::string>& args, std::ostream& out, std::ostream& err)
{
	Parser parser = { out, err, args[0] };

	std::deque<std::string> arg_queue;
	for (const std::string& v : args)
		arg_queue.push_back(v);
	arg_queue.pop_front();

	return parser.parse(arg_queue, status);
}
