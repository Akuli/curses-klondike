/*
non-gnu-extension getopt doesn't support --long-arguments
so i did it myself, why not :D
doesn't support --, but nobody needs it for this program imo
*/

#include <memory>
#include <set>
#include <deque>
#include <string>
#include <optional>
#include <vector>
#include <stdexcept>
#include "args.hpp"

enum class OptionType { YESNO, INT };

struct OptionSpec {
	std::string_view name;
	std::optional<std::string_view> metavar;
	OptionType type;
	int min;
	int max;
	std::string_view desc;

	std::string get_name_with_metavar(char sep = ' ') const {
		if (this->metavar.has_value())
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
	Parser(FILE *out, FILE *err, std::string argv0) : out(out), err(err), argv0(argv0) {}

	std::optional<Args> parse(std::deque<std::string>& args, int& status) const
	{
		bool tokenize_ok = true;
		std::vector<Token> tokens = {};
		while (!args.empty()) {
			std::optional<Token> token = this->get_token(args);
			if (!token.has_value()) {
				tokenize_ok = false;
				break;
			}
			tokens.push_back(token.value());
		}

		if (!tokenize_ok || !this->check_tokens(tokens)) {
			std::fprintf(this->err, "Run '%s --help' for help.\n", this->argv0.c_str());
			status = 2;
			return std::nullopt;
		}

		std::optional<Args> res = this->tokens_to_struct_args(tokens);
		if (!res.has_value())
			status = 0;  // help printed (success)
		return res;
	}

private:
	FILE *out;
	FILE *err;
	std::string argv0;

	void print_help() const
	{
		std::fprintf(this->out, "Usage: %s", this->argv0.c_str());
		for (const OptionSpec& opt : option_specs)
			std::fprintf(this->out, " [%s]", std::string(opt.get_name_with_metavar()).c_str());

		std::fprintf(this->out, "\n\nOptions:\n");
		for (const OptionSpec& opt : option_specs)
		{
			std::fprintf(this->out, "  %-*s  %s\n",
				option_max_length,
				opt.get_name_with_metavar().c_str(),
				std::string(opt.desc).c_str());
		}
	}

	std::optional<OptionSpec> find_from_option_specs(std::string nam) const
	{
		std::optional<OptionSpec> res = std::nullopt;

		for (const OptionSpec& spec : option_specs) {
			if (spec.name.find(nam) != 0)
				continue;

			if (res.has_value()) {
				std::fprintf(this->err, "%s: ambiguous option '%s': could be '%s' or '%s'\n",
					this->argv0.c_str(),
					nam.c_str(),
					std::string(res.value().name).c_str(),
					std::string(spec.name).c_str());
				return std::nullopt;
			}
			res = spec;
		}

		if (!res.has_value())
			std::fprintf(this->err, "%s: unknown option '%s'\n", this->argv0.c_str(), nam.c_str());
		return res;
	}

	// argc and argv should point to remaining args, not always all the args
	std::optional<Token> get_token(std::deque<std::string>& argq) const
	{
		std::string arg = argq[0];
		argq.pop_front();

		if (arg.length() < 3 || arg.find("--") != 0) {
			std::fprintf(this->err, "%s: unexpected argument: '%s'\n", this->argv0.c_str(), arg.c_str());
			return std::nullopt;
		}

		std::string nam;
		std::optional<std::string> val;

		size_t i = arg.find("=");
		if (i != std::string::npos) {
			nam = arg.substr(0, i);
			val = arg.substr(i+1);
		} else {
			nam = arg;
			if (argq.empty() || argq[0].c_str()[0] == '-') {
				val = std::nullopt;
			} else {
				val = argq[0];
				argq.pop_front();
			}
		}

		std::optional<OptionSpec> spec = this->find_from_option_specs(nam.c_str());
		if (!spec.has_value())
			return std::nullopt;
		return Token{ spec.value(), val };
	}

	bool check_token_by_type(const Token& tok) const
	{
		switch(tok.spec.type) {
		case OptionType::YESNO:
			if (tok.value.has_value()) {
				std::fprintf(this->err, "%s: use just '%s', not '%s=something' or '%s something'\n",
					this->argv0.c_str(),
					std::string(tok.spec.name).c_str(), std::string(tok.spec.name).c_str(), std::string(tok.spec.name).c_str());
				return false;
			}
			break;

		case OptionType::INT:
			if (!tok.value.has_value()) {
				std::fprintf(this->err, "%s: use '%s' or '%s', not just '%s'\n",
					this->argv0.c_str(),
					tok.spec.get_name_with_metavar(' ').c_str(),
					tok.spec.get_name_with_metavar('=').c_str(),
					std::string(tok.spec.name).c_str());
				return false;
			}
			if (!is_valid_integer(tok.value.value(), tok.spec.min, tok.spec.max)) {
				std::fprintf(this->err, "%s: '%s' wants an integer between %d and %d, not '%s'\n",
					this->argv0.c_str(),
					std::string(tok.spec.name).c_str(),
					tok.spec.min, tok.spec.max,
					tok.value.value().c_str());
				return false;
			}
			break;
		}

		return true;
	}

	bool check_tokens(const std::vector<Token>& tokens) const
	{
		std::set<std::string_view> seen = {};

		for (Token tok : tokens) {
			if (seen.count(tok.spec.name) > 0) {
				std::fprintf(this->err, "%s: repeated option '%s'\n",
					this->argv0.c_str(),
					std::string(tok.spec.name).c_str());
				return false;
			}
			seen.insert(tok.spec.name);

			if (!this->check_token_by_type(tok))
				return false;
		}

		return true;
	}

	// returns true to keep running, or false to exit with status 0
	std::optional<Args> tokens_to_struct_args(const std::vector<Token>& tokens) const
	{
		Args ar = {};
		for (Token tok : tokens) {
			if (tok.spec.name == "--help") {
				this->print_help();
				return std::nullopt;
			}

			if (tok.spec.name == "--no-colors")
				ar.color = false;
			else if (tok.spec.name == "--pick")
				ar.pick = std::stoi(tok.value.value());  // already validated
			else if (tok.spec.name == "--discard-hide")
				ar.discardhide = true;
			else
				throw std::logic_error("unknown arg name: " + std::string(tok.spec.name));
		}
		return ar;
	}

};

std::optional<Args> args_parse(int& status, const std::vector<std::string>& args, FILE *out, FILE *err)
{
	Parser parser = { out, err, args[0] };

	std::deque<std::string> arg_queue;
	for (const std::string& v : args)
		arg_queue.push_back(v);
	arg_queue.pop_front();

	return parser.parse(arg_queue, status);
}
