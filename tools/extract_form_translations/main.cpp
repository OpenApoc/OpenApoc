#include "dependencies/pugixml/src/pugixml.hpp"
#include "fmt/core.h"
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

constexpr std::string_view header = R"(
# OpenApoc form translations
# Copyright (C) YEAR The OpenApoc team
# This file is distributed under the same license as the OpenApoc package.
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: OpenApoc 0.1\n"
"Language: en\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
)";

namespace po = boost::program_options;

struct message
{
	std::string text;
};

static const char *text_attribute_name = "text";

struct message_walker : pugi::xml_tree_walker
{
	std::vector<message> messages;
	bool for_each(pugi::xml_node &node) override
	{
		auto attr = node.attribute(text_attribute_name);
		if (attr)
		{
			std::string str = attr.as_string();
			if (str != "")
			{
				message m;
				m.text = str;
				messages.push_back(m);
			}
		}
		return true;
	}
};

int main(int argc, char **argv)
{
	po::options_description desc("Allowed options");
	// clang-format off
	desc.add_options()
		("help", "Show help message")
		("output-pofile,o", po::value<std::string>(), "Path of output pofile (- means stdout)")
		("input-file,i", po::value<std::vector<std::string>>(), "Input form file");
	// clang-format on

	po::positional_options_description p;
	p.add("input-file", -1);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		std::cout << desc << "\n";
		return EXIT_SUCCESS;
	}
	if (!vm.count("output-pofile"))
	{
		std::cerr << "Must specify output pofile\n";
		return EXIT_FAILURE;
	}
	if (!vm.count("input-file"))
	{
		std::cerr << "Must specify at least one input form\n";
		return EXIT_FAILURE;
	}

	const auto input_files = vm["input-file"].as<std::vector<std::string>>();
	const auto output_file = vm["output-pofile"].as<std::string>();

	std::vector<message> messages;
	std::map<std::string, std::vector<message>> file_messages;

	for (const auto &formfile : input_files)
	{
		std::ifstream in(formfile);
		if (!in)
		{
			std::cerr << fmt::format("Failed to open input file \"{0}\"\n", formfile);
			return EXIT_FAILURE;
		}
		pugi::xml_document doc;
		auto parse_result = doc.load(in);
		if (!parse_result)
		{
			std::cerr << fmt::format("Failed to parse input file \"{0}\": \"{1}\"\n", formfile,
			                         parse_result.description());
			return EXIT_FAILURE;
		}
		message_walker walker;
		doc.traverse(walker);
		file_messages[formfile] = std::move(walker.messages);
	}

	std::ostringstream output;

	output << header;

	// Remove duplicates - so it's now <message, file list>
	std::map<std::string, std::vector<std::string>> deduplicated_messages;

	for (const auto [file, messages] : file_messages)
	{
		for (const auto &message : messages)
		{
			deduplicated_messages[message.text].push_back(file);
		}
	}

	for (const auto [message, containing_files] : deduplicated_messages)
	{
		for (const auto &file : containing_files)
			output << fmt::format("#: {0}\n", file);
		output << fmt::format("msgid \"{0}\"\n", message);
		output << fmt::format("msgstr \"\"\n\n");
	}

	if (output_file == "-")
		std::cout << output.str();
	else
	{
		std::ofstream outstream(output_file);
		if (!outstream)
		{
			std::cerr << fmt::format("Failed to open output file \"{0}\"\n", output_file);
			return EXIT_FAILURE;
		}
		outstream << output.str();
	}

	return 0;
}
