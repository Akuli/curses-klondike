/*
non-gnu-extension getopt doesn't support --long-arguments
so i did it myself, why not :D
doesn't support --, but nobody needs it for this program imo
*/

#include <deque>
#include <string>
#include <optional>
#include <vector>
#include <stdexcept>
#include "args.hpp"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.hpp"

enum class OptType { YESNO, INT };

struct OptSpec {
	std::string name;
	const char *metavar;  // TODO: figure out how to make optional string
	OptType type;
	int min;
	int max;
	std::string desc;
};

// TODO: document env vars in --help
static const OptSpec option_specs[] = {
	{ "--help", nullptr, OptType::YESNO, 0, 0, "show this help message and exit" },
	{ "--no-colors", nullptr, OptType::YESNO, 0, 0, "don't use colors, even if the terminal supports colors" },
	{ "--pick", "n", OptType::INT, 1, 13*4 - (1+2+3+4+5+6+7), "pick n cards from stock at a time, default is 3" },
	{ "--discard-hide", nullptr, OptType::YESNO, 0, 0, "only show topmost discarded card (not useful with --pick=1)" }
};
static std::string longest_option = "--discard-hide";   // TODO: replace this with a function that calculates length

struct Printer {
	FILE *out;
	FILE *err;
	std::string argv0;
};

static void print_help_option(Printer printer, OptSpec opt)
{
	std::string pre = opt.name;
	if (opt.type == OptType::INT) {
		pre += " ";
		pre += opt.metavar;
	}

	std::fprintf(printer.out, "  %-*s  %s\n", (int)longest_option.length(), pre.c_str(), opt.desc.c_str());
}

static void print_help(Printer printer)
{
	std::fprintf(printer.out, "Usage: %s", printer.argv0.c_str());
	for (OptSpec spec : option_specs) {
		std::string s = spec.name;
		if (spec.type == OptType::INT) {
			s += " ";
			s += spec.metavar;
		}
		std::fprintf(printer.out, " [%s]", s.c_str());
	}

	std::fprintf(printer.out, "\n\nOptions:\n");
	for (OptSpec spec : option_specs)
		print_help_option(printer, spec);
}

// represents an option and a value, if any
// needed because one arg token can come from 2 argv items: --pick 3
// or just 1: --no-colors, --pick=3
struct Token {
	const OptSpec *spec;  // pointer to an item of option_specs
	const char *value;  // TODO: figure out how to do string or null
};

static const OptSpec *find_from_option_specs(Printer printer, const char *nam)
{
	const OptSpec *res = nullptr;
	for (unsigned int i = 0; i < sizeof(option_specs)/sizeof(option_specs[0]); i++) {
		if (option_specs[i].name.find(nam) != 0)
			continue;

		if (res) {
			std::fprintf(printer.err, "%s: ambiguous option '%s': could be '%s' or '%s'\n",
					printer.argv0.c_str(), nam, res->name.c_str(), option_specs[i].name.c_str());
			return nullptr;
		}
		res = &option_specs[i];
	}

	if (!res)
		std::fprintf(printer.err, "%s: unknown option '%s'\n", printer.argv0.c_str(), nam);
	return res;
}

// argc and argv should point to remaining args, not always all the args
static bool tokenize(Printer printer, Token& tok, std::deque<std::string>& argq)
{
	std::string arg = argq[0];
	argq.pop_front();

	if (arg.length() < 3 || arg.find("--") != 0) {
		std::fprintf(printer.err, "%s: unexpected argument: '%s'\n", printer.argv0.c_str(), arg.c_str());
		return false;
	}

	std::string nam;
	char *val;  // TODO: std::optional<std::string>

	size_t i = arg.find("=");
	if (i != std::string::npos) {
		nam = arg.substr(0, i);
		std::string valstring = arg.substr(i+1);
		val = (char*)malloc(valstring.length() + 1);
		assert(val);
		strcpy(val, valstring.c_str());
	} else {
		nam = arg;
		if (argq.empty() || argq[0].c_str()[0] == '-') {
			val = nullptr;
		} else {
			std::string valarg = argq[0];
			argq.pop_front();

			// TODO: handle negative numbers as arguments?
			val = (char*)malloc(valarg.length() + 1);
			assert(val);
			strcpy(val, valarg.c_str());
		}
	}

	const OptSpec *spec = find_from_option_specs(printer, nam.c_str());
	if (!spec)
		return false;

	tok.spec = spec;
	tok.value = val;
	return true;
}

static bool is_valid_integer(std::string str, int min, int max)
{
	try {
		int n = std::stoi(str);
		return min <= n && n <= max;
	} catch(...) {
		return false;
	}
}

// returns 0 on success, negative on failure
static int check_token_by_type(Printer printer, Token tok)
{
	switch(tok.spec->type) {
	case OptType::YESNO:
		if (tok.value) {
			std::fprintf(printer.err, "%s: use just '%s', not '%s=something' or '%s something'\n",
					printer.argv0.c_str(), tok.spec->name.c_str(), tok.spec->name.c_str(), tok.spec->name.c_str());
			return -1;
		}
		break;

	case OptType::INT:
		if (!tok.value) {
			std::fprintf(printer.err, "%s: use '%s %s' or '%s=%s', not just '%s'\n",
					printer.argv0.c_str(), tok.spec->name.c_str(), tok.spec->metavar, tok.spec->name.c_str(), tok.spec->metavar, tok.spec->name.c_str());
			return -1;
		}
		if (!is_valid_integer(tok.value, tok.spec->min, tok.spec->max)) {
			std::fprintf(printer.err, "%s: '%s' wants an integer between %d and %d, not '%s'\n",
					printer.argv0.c_str(), tok.spec->name.c_str(), tok.spec->min, tok.spec->max, tok.value);
			return -1;
		}
		break;
	}

	return 0;
}

// returns 0 on success, negative on failure
static bool check_tokens(Printer printer, const Token *toks, int ntoks)
{
	// to detect duplicates
	bool seen[sizeof(option_specs)/sizeof(option_specs[0])] = {0};

	for (int i = 0; i < ntoks; i++) {
		int j = toks[i].spec - &option_specs[0];
		if (seen[j]) {
			std::fprintf(printer.err, "%s: repeated option '%s'\n", printer.argv0.c_str(), toks[i].spec->name.c_str());
			return false;
		}
		seen[j] = true;

		if (check_token_by_type(printer, toks[i]) < 0)
			return false;
	}

	return true;
}


// returns true to keep running, or false to exit with status 0
static bool tokens_to_struct_args(Printer printer, const Token *toks, int ntoks, Args& ar)
{
	ar = {};
	for (int i = 0; i < ntoks; i++) {

		if (toks[i].spec->name == "--help") {
			print_help(printer);
			return false;
		}

		if (toks[i].spec->name == "--no-colors")
			ar.color = false;
		else if (toks[i].spec->name == "--pick")
			ar.pick = std::stoi(toks[i].value);  // value already validated
		else if (toks[i].spec->name == "--discard-hide")
			ar.discardhide = true;
		else
			assert(0);
	}

	return true;
}

int args_parse(Args& ar, std::vector<std::string> argvec, FILE *out, FILE *err)
{
	Printer printer = { out, err, argvec[0] };

	std::deque<std::string> argq;
	for (std::string v : argvec)
		argq.push_back(v);

	std::string argv0 = argq[0];
	argq.pop_front();

	std::vector<Token> toks = {};
	while (!argq.empty()) {
		Token t;
		if (!tokenize(printer, t, argq)) {
			// TODO: use some kind of smart pointer shit instead of copy/pasta
			std::fprintf(printer.err, "Run '%s --help' for help.\n", printer.argv0.c_str());
			return 2;
		}
		toks.push_back(t);
	}

	if (!check_tokens(printer, toks.data(), toks.size())) {
		// TODO: use some kind of smart pointer shit instead of copy/pasta
		std::fprintf(printer.err, "Run '%s --help' for help.\n", printer.argv0.c_str());
		return 2;
	}

	return tokens_to_struct_args(printer, toks.data(), toks.size(), ar) ? -1 : 0;
}
