/*
Non-gnu-extension getopt doesn't support --long-arguments, so i did it myself, why not :D

Not very general, doesn't support various features that this program doesn't need.

Note about error handling: Most functions print an error message and return
something to indicate failure (e.g. false, -1). Then a message like
"Run './cursesklon --help' for help" is printed in one place, so that every
error message gets it added when the error indicator is returned.
*/

#include "args.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

enum OptionType { OPT_YESNO, OPT_INT };

struct OptionSpec {
	const char *name;
	const char *metavar;  // may be NULL
	enum OptionType type;
	int min, max;	// Meaningful only for integer options
	const char *desc;
};

/*
Print e.g. "--pick n" or "--help".
*/
static void print_name_and_metavar(const struct OptionSpec *optspec, FILE *dest, char sep)
{
	if (optspec->metavar)
		fprintf(dest, "%s%c%s", optspec->name, sep, optspec->metavar);
	else
		fprintf(dest, "%s", optspec->name);
}

static const struct OptionSpec option_specs[] = {
	{ .name = "--help", .type = OPT_YESNO, .desc = "show this help message and exit" },
	{ .name = "--no-colors", .type = OPT_YESNO, .desc = "don't use colors, even if the terminal supports colors" },
	{ .name = "--pick", .type = OPT_INT, .metavar = "n", .min = 1, .max = 13*4 - (1+2+3+4+5+6+7), .desc = "pick n cards from stock at a time, default is 3" },
	{ .name = "--discard-hide", .type = OPT_YESNO, .desc = "only show topmost discarded card (not useful with --pick=1)" }
};

#define OPTION_MAX_LENGTH (sizeof("--discard-hide") - 1)

/*
Represents an option and its value, if any.

This comes from one or two command-line (argv) arguments. For example:

	--pick 3	--> Token{.spec = ..., .value = "3"}
	--pick=3	--> Token{.spec = ..., .value = "3"}
	--no-colors --> Token{.spec = ..., .value = NULL}
*/
struct Token {
	const struct OptionSpec *spec;  // pointer into option_specs array
	const char *value;  // may be NULL
};

/*
Tests if a string represents an integer that is between the minimum and maximum bounds.
*/
static bool is_valid_integer(const char *str, int min, int max)
{
	for (int i = 0; str[i] != '\0'; i++)
		if (!('0' <= str[i] && str[i] <= '9'))
			return false;

	int n = atoi(str);
	return min <= n && n <= max;
}

struct ArgParser {
	FILE *out;  // stdout or temporary file during tests
	FILE *err;  // stdout or temporary file during tests
	const char *argv0;  // first command line argument (program name)
};

struct Args default_args = {
	.color = true,
	.pick = 3,
	.discardhide = false,
};

/*
Determine whether a string starts with a prefix.
*/
static bool starts_with(const char *str, const char *prefix)
{
	return !strncmp(str, prefix, strlen(prefix));
}

/*
Creates one from command-line args.

argv does not include argv0 and is incremented to skip processed arguments.

Return value false indicates error. In that case an error message has been
printed, but the "Run './cursesklon --help' for help" common to all errors is
not yet printed.
*/
static bool get_token(const struct ArgParser *parser, struct Token *dest, const char *const **argv)
{
	const char *first_arg = **argv;
	assert(first_arg != NULL);
	++*argv;

	if (strlen(first_arg) < 3 || !starts_with(first_arg, "--")) {
		fprintf(parser->err, "%s: unexpected argument '%s'\n", parser->argv0, first_arg);
		return false;
	}

	char *name = strdup(first_arg);
	const char *value;

	const char *eq = strstr(name, "=");
	if (eq) {
		*eq = '\0';  // truncate the name
		value = eq+1;
	} else {
		const char *next_arg = **argv;
		if (next_arg == NULL || starts_with(next_arg, "-")) {
			// No more arguments, or next argument is another option
			// e.g. "--discardhide --pick=3" sees the initial '-' of "--pick=3"
			value = NULL;
		} else {
			value = next_arg;
			++*argv;
		}
	}

	const OptionSpec *spec = find_from_option_specs(name);
	free(name);

	if (!spec)
		return false;

	switch(spec->type) {
		case OPT_YESNO:
			if (value) {
				fprintf(
					parser->err,
					"%s: use just '%s', not '%s something' or '%s=something'",
					parser->argv0, spec->name, spec->name, spec->name);
				return false;
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

/*
Creates tokens from command-line args.

argv does not include argv0.

Return value -1 indicates error. In that case an error message has been
printed, but the "Run './cursesklon --help' for help" common to all errors is
not yet printed.
*/
static int tokenize(const struct ArgParser *parser, struct Token *tokens, size_t max_tokens, const char *const *argv)
{
	int ntokens = 0;
	while (*argv != NULL) {
		struct Token t;
		if (!get_token(&t, &argv))
			return -1;

		if (ntokens == max_tokens) {
			// TODO: test this
			fprintf(parser->err, "%s: too many command-line arguments\n");
			return -1;
		}

		tokens[ntokens++] = t;
	}
}

int parse_args(struct Args *args_out, const char *const *argv_in, FILE *out, FILE *err)
{
	struct ArgParser parser = { .out = out, .err = err, .argv0 = *argv_in++ };
	assert(parser.argv0 != NULL);

	*args_out = default_args;

	bool tokenize_ok = true;
	struct Token tokens[100];
	int ntokens = 0;

	while (ntokens < sizeof(tokens)/sizeof(tokens[0]) && *argv_in != NULL) {
		if (ntokens == sizeof(tokens)/sizeof(tokens[0])) {
			fprintf(parser.err, ""
			this->err << this->argv0 << ": repeated option '" << *duplicate << "'" << std::endl;
			this->err << this->argv0 << ": repeated option '" << *duplicate << "'" << std::endl;
			fprintf(
	}
		struct Token t;
		if (!get_token(&t, &argv_in)) {
			tokenize_ok = false;
			break;
		}
	}
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
