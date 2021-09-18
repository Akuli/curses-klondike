/*
non-gnu-extension getopt doesn't support --long-arguments
so i did it myself, why not :D
doesn't support --, but nobody needs it for this program imo
*/

#include <set>
#include <deque>
#include <string>
#include <optional>
#include <vector>
#include <stdexcept>
#include "args.hpp"
#include <stdio.h>
#include "misc.hpp"

enum class OptType { YESNO, INT };

struct OptSpec {
	std::string name;
	std::optional<std::string> metavar;
	OptType type;
	int min;
	int max;
	std::string desc;
};

// TODO: document env vars in --help
static const std::vector<OptSpec> option_specs = {
	{ "--help", std::nullopt, OptType::YESNO, 0, 0, "show this help message and exit" },
	{ "--no-colors", std::nullopt, OptType::YESNO, 0, 0, "don't use colors, even if the terminal supports colors" },
	{ "--pick", "n", OptType::INT, 1, 13*4 - (1+2+3+4+5+6+7), "pick n cards from stock at a time, default is 3" },
	{ "--discard-hide", std::nullopt, OptType::YESNO, 0, 0, "only show topmost discarded card (not useful with --pick=1)" }
};
static const std::string longest_option = "--discard-hide";   // TODO: replace this with a function that calculates length

struct Printer {
	FILE *out;
	FILE *err;
	std::string argv0;
};

static void print_help_option(Printer printer, OptSpec opt)
{
	std::string pre = opt.name;
	if (opt.metavar.has_value())
		pre += " " + opt.metavar.value();

	std::fprintf(printer.out, "  %-*s  %s\n", (int)longest_option.length(), pre.c_str(), opt.desc.c_str());
}

static void print_help(Printer printer)
{
	std::fprintf(printer.out, "Usage: %s", printer.argv0.c_str());
	for (OptSpec spec : option_specs) {
		std::string s = spec.name;
		if (spec.metavar.has_value())
			s += " " + spec.metavar.value();
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
	OptSpec spec;
	std::optional<std::string> value;
};

static std::optional<OptSpec> find_from_option_specs(Printer printer, std::string nam)
{
	std::optional<OptSpec> res = std::nullopt;

	for (OptSpec spec : option_specs) {
		if (spec.name.find(nam) != 0)
			continue;

		if (res.has_value()) {
			std::fprintf(printer.err, "%s: ambiguous option '%s': could be '%s' or '%s'\n",
					printer.argv0.c_str(), nam.c_str(), res->name.c_str(), spec.name.c_str());
			return std::nullopt;
		}
		res = spec;
	}

	if (!res.has_value())
		std::fprintf(printer.err, "%s: unknown option '%s'\n", printer.argv0.c_str(), nam.c_str());
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

	std::optional<OptSpec> spec = find_from_option_specs(printer, nam.c_str());
	if (!spec.has_value())
		return false;

	tok.spec = spec.value();
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

static bool check_token_by_type(Printer printer, Token tok)
{
	switch(tok.spec.type) {
	case OptType::YESNO:
		if (tok.value.has_value()) {
			std::fprintf(printer.err, "%s: use just '%s', not '%s=something' or '%s something'\n",
					printer.argv0.c_str(), tok.spec.name.c_str(), tok.spec.name.c_str(), tok.spec.name.c_str());
			return false;
		}
		break;

	case OptType::INT:
		if (!tok.value.has_value()) {
			std::fprintf(printer.err, "%s: use '%s %s' or '%s=%s', not just '%s'\n",
					printer.argv0.c_str(),
					tok.spec.name.c_str(), tok.spec.metavar.value().c_str(),
					tok.spec.name.c_str(), tok.spec.metavar.value().c_str(),
					tok.spec.name.c_str());
			return false;
		}
		if (!is_valid_integer(tok.value.value(), tok.spec.min, tok.spec.max)) {
			std::fprintf(printer.err, "%s: '%s' wants an integer between %d and %d, not '%s'\n",
					printer.argv0.c_str(), tok.spec.name.c_str(), tok.spec.min, tok.spec.max, tok.value.value().c_str());
			return false;
		}
		break;
	}

	return true;
}

static bool check_tokens(Printer printer, std::vector<Token> toks)
{
	std::set<std::string> seen = {};

	for (Token tok : toks) {
		if (seen.count(tok.spec.name) > 0) {
			std::fprintf(printer.err, "%s: repeated option '%s'\n", printer.argv0.c_str(), tok.spec.name.c_str());
			return false;
		}
		seen.insert(tok.spec.name);

		if (!check_token_by_type(printer, tok))
			return false;
	}

	return true;
}


// returns true to keep running, or false to exit with status 0
static bool tokens_to_struct_args(Printer printer, std::vector<Token> toks, Args& ar)
{
	ar = {};
	for (Token tok : toks) {
		if (tok.spec.name == "--help") {
			print_help(printer);
			return false;
		}

		if (tok.spec.name == "--no-colors")
			ar.color = false;
		else if (tok.spec.name == "--pick")
			ar.pick = std::stoi(tok.value.value());  // already validated
		else if (tok.spec.name == "--discard-hide")
			ar.discardhide = true;
		else
			throw std::logic_error("unknown arg name: " + tok.spec.name);
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

	bool tokenize_ok = true;
	std::vector<Token> toks = {};
	while (!argq.empty()) {
		Token t;
		if (!tokenize(printer, t, argq)) {
			tokenize_ok = false;
			break;
		}
		toks.push_back(t);
	}

	if (!tokenize_ok || !check_tokens(printer, toks)) {
		std::fprintf(printer.err, "Run '%s --help' for help.\n", printer.argv0.c_str());
		return 2;
	}

	return tokens_to_struct_args(printer, toks, ar) ? -1 : 0;
}
