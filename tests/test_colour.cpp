#include "framework/configfile.h"
#include "framework/logger.h"
#include "library/colour.h"
#include <list>

using namespace OpenApoc;

int main(int argc, char **argv)
{
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	std::list<Colour> expected{{255, 255, 255}, {192, 192, 192}, {128, 128, 128}, {0, 0, 0},
	                           {255, 0, 0},     {128, 0, 0},     {255, 255, 0},   {128, 128, 0},
	                           {0, 255, 0},     {0, 128, 0},     {0, 255, 255},   {0, 128, 128},
	                           {0, 0, 255},     {0, 0, 128},     {255, 0, 255},   {128, 0, 128}};

	std::list<Colour> name_parsed{Colour::FromHtmlName("WhItE"),   Colour::FromHtmlName("SiLvEr"),
	                              Colour::FromHtmlName("GrAy"),    Colour::FromHtmlName("bLaCk"),
	                              Colour::FromHtmlName("rEd"),     Colour::FromHtmlName("mArOoN"),
	                              Colour::FromHtmlName("yEllOw"),  Colour::FromHtmlName("Olive"),
	                              Colour::FromHtmlName("LiMe"),    Colour::FromHtmlName("GrEEn"),
	                              Colour::FromHtmlName("Aqua"),    Colour::FromHtmlName("Teal"),
	                              Colour::FromHtmlName("Blue"),    Colour::FromHtmlName("NavY"),
	                              Colour::FromHtmlName("FuchSIA"), Colour::FromHtmlName("Purple")};

	std::list<Colour> hex_parsed{
	    Colour::FromHex("#fff"),    Colour::FromHex("#C0C0C0"), Colour::FromHex("#808080"),
	    Colour::FromHex("#000"),    Colour::FromHex("#f00"),    Colour::FromHex("#800000"),
	    Colour::FromHex("#ff0"),    Colour::FromHex("#808000"), Colour::FromHex("#0f0"),
	    Colour::FromHex("#008000"), Colour::FromHex("#0ff"),    Colour::FromHex("#008080"),
	    Colour::FromHex("#00f"),    Colour::FromHex("#000080"), Colour::FromHex("#f0f"),
	    Colour::FromHex("#800080")};

	auto it_expected = expected.begin();
	auto it_test = name_parsed.begin();
	while (it_expected != expected.end() && it_test != name_parsed.end())
	{
		if (*it_expected != *it_test)
		{
			LogError("Colour parsing by name mismatch: expected (%d, %d, %d), got (%d, %d, %d)",
			         it_expected->r, it_expected->g, it_expected->b, it_test->r, it_test->g,
			         it_test->b);
			return EXIT_FAILURE;
		}
		++it_expected;
		++it_test;
	}

	it_expected = expected.begin();
	it_test = hex_parsed.begin();
	while (it_expected != expected.end() && it_test != hex_parsed.end())
	{
		if (*it_expected != *it_test)
		{
			LogError("Colour parsing by name mismatch: expected (%d, %d, %d), got (%d, %d, %d)",
			         it_expected->r, it_expected->g, it_expected->b, it_test->r, it_test->g,
			         it_test->b);
			return EXIT_FAILURE;
		}
		++it_expected;
		++it_test;
	}
	return EXIT_SUCCESS;
}
