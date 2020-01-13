#include "dependencies/pugixml/src/pugixml.hpp"

#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <cctype>

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

void read_tooltip(pugi::xml_node &node, std::set<std::string> &strings)
{
	std::string_view text = node.attribute("text").as_string();
	if (!text.empty())
	{
		strings.emplace(text);
	}
}

void read_control(pugi::xml_node &node, std::set<std::string> &strings)

{
	const std::string_view node_name = node.name();

	if (node_name == "label" || node_name == "textbutton" || node_name == "textedit")
	{
		std::string_view text = node.attribute("text").as_string();
		if (!text.empty())
		{
			strings.emplace(text);
		}
	}

	for (auto child : node.children())
	{
		const std::string_view child_name = child.name();
		if (child_name == "control")
		{
			read_control(child, strings);
		}
		if (child_name == "tooltip")
		{
			read_tooltip(child, strings);
		}
	}
}

void read_style(pugi::xml_node &node, std::set<std::string> &strings)
{
	for (auto child : node.children())
	{
		read_control(child, strings);
	}
}

void read_form(pugi::xml_node &node, std::set<std::string> &strings)
{
	for (auto child : node.children("style"))
	{
		read_style(child, strings);
	}
}

int main(int argc, char **argv)
{
	std::set<std::string> strings;

	for (int i = 1; i < argc; i++)
	{
		pugi::xml_document doc;
		auto result = doc.load_file(argv[i]);
		if (!result)
		{
			std::cerr << "Failed to read form \"" << argv[i] << "\" : \"" << result.description()
			          << "\"\n";
			return EXIT_FAILURE;
		}
		auto node = doc.child("openapoc");
		if (!node)
		{
			std::cerr << "No \"openapoc\" root element in form file \"" << argv[i] << "\"\n";
			return EXIT_FAILURE;
		}

		auto child = node.child("form");
		if (!child)
		{
			std::cerr << "No \"openapoc.form\" element in form file \"" << argv[i] << "\"\n";
			return EXIT_FAILURE;
		}

		read_form(child, strings);
	}

	std::cout << header;

	for (const auto &string : strings)
	{
		const auto escaped_string = escape_string(string);
		std::cout << "msgid \"" << escaped_string << "\"\n";
		std::cout << "msgstr \"\"\n\n";
	}

	return EXIT_SUCCESS;
}
