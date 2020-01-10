#include "framework/configfile.h"
#include "framework/filesystem.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize.h"
#include <iostream>
#include <sstream>
#include <string_view>

const auto header = R"foo(msgid ""
msgstr ""
"Project-Id-Version: OpenApoc\n"
"POT-Creation-Date: \n"
"PO-Revision-Date: 2020-01-04 16:20+0800\n"
"Last-Translator: JonnyH\n"
"Language-Team: English (United Kingdom) (http://www.transifex.com/x-com-apocalypse/apocalypse/language/en_GB/)\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
"Language: en_GB\n"

)foo";

static std::string escape_character(const char ch)
{
	if (ch == '"')
		return "\\\"";
	if (std::isprint(ch))
		return std::string{ch};
	else
	{
		std::cerr << "Unknown char 0x" << std::hex << (int)ch << "\n";
		return "";
	}
}

static std::string escape_string(const std::string_view str)
{
	std::string escaped;
	for (const auto ch : str)
	{
		escaped += escape_character(ch);
	}

	return escaped;
}

int main(int argc, char **argv)
{
	OpenApoc::config().addPositionalArgument("input", "Input file");

	if (OpenApoc::config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	auto input1 = OpenApoc::config().getString("input");
	if (input1.empty())
	{
		std::cerr << "Must provide at least input\n";
		OpenApoc::config().showHelp();
		return EXIT_FAILURE;
	}

	const auto state_path = OpenApoc::config().getString("input");

	OpenApoc::Framework fw("OpenApoc", false);

	auto state = OpenApoc::mksp<OpenApoc::GameState>();
	state->populate_translateable_strings = true;
	if (!state->loadGame(state_path))
	{
		std::cerr << "Failed to load input file \"" << state_path << "\"\n";
		return EXIT_FAILURE;
	}

	std::cout << header;

	for (const auto &string : state->translateable_strings)
	{
		const auto escaped_string = escape_string(string.cStr());
		std::cout << "msgid \"" << escaped_string << "\"\n";
		std::cout << "msgstr \"\"\n\n";
	}

	return EXIT_SUCCESS;
}
