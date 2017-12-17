#include "framework/configfile.h"
#include "framework/logger.h"
#include "library/strings.h"

using namespace OpenApoc;

struct example_unicode
{
	const char *u8string;
	std::vector<UniChar> expected_codepoints;

	example_unicode(const char *str, std::vector<UniChar> codepoints)
	    : u8string(str), expected_codepoints(codepoints){};

	bool test() const
	{
		LogInfo("Testing string \"%s\"", u8string);
		const auto num_codepoints = expected_codepoints.size();
		UString string(u8string);
		UString string2;
		if (string.length() != num_codepoints)
		{
			LogError("String \"%s\" has unexpected length %zu , expected %zu", u8string,
			         string.length(), num_codepoints);
			return false;
		}

		std::vector<UniChar> decoded_codepoints;

		for (auto c : string)
			decoded_codepoints.push_back(c);

		if (decoded_codepoints.size() != num_codepoints)
		{
			LogError("String \"%s\" has unexpected iterated length %zu , expected %zu", u8string,
			         decoded_codepoints.size(), num_codepoints);
			return false;
		}

		for (size_t i = 0; i < num_codepoints; i++)
		{
			if (expected_codepoints[i] != decoded_codepoints[i])
			{
				LogError(
				    "String \"%s\" has unexpected codepoint at index %zu - got 0x%x expected 0x%x",
				    u8string, i, decoded_codepoints[i], expected_codepoints[i]);
				return false;
			}
			string2 += decoded_codepoints[i];
		}

		if (string != string2)
		{
			LogError(
			    "String \"%s\" compared false after utf8->unichar->utf8 conversion - got \"%s\"",
			    u8string, string2);

			return false;
		}

		LogInfo("Looks good");

		return true;
	}
};

static bool test_remove(const UString &initial, const UString &expected, size_t offset,
                        size_t count)
{
	UString removed = initial;
	removed.remove(offset, count);
	if (removed != expected)
	{
		LogError("\"%s\".remove(%zu, %zu) = \"%s\", expected \"%s\"", initial, offset, count,
		         removed, expected);
		return false;
	}
	return true;
}

static bool test_insert(const UString &initial, const UString &expected, size_t offset,
                        const UString &str)
{
	UString inserted = initial;
	inserted.insert(offset, str);
	if (inserted != expected)
	{
		LogError("\"%s\".inserted(%zu, \"%s\") = \"%s\", expected \"%s\"", initial, offset, str,
		         inserted, expected);
		return false;
	}
	return true;
}

int main(int argc, char **argv)
{
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	const std::vector<example_unicode> examples = {
	    {u8"£", {0xA3}},
	    {u8"a", {0x61}},
	    {u8"€", {0x20AC}},
	    {u8"𐍈", {0x10348}},
	    {u8"£a€𐍈", {0xA3, 0x61, 0x20AC, 0x10348}},
	};

	for (const auto &ex : examples)
	{
		if (ex.test() != true)
			return EXIT_FAILURE;
	}

	UString example = "€UPpa91£B\"#ð𐍈";
	UString lower_example = "€uppa91£b\"#ð𐍈";
	UString upper_example = "€UPPA91£B\"#ð𐍈";

	auto lower = example.toLower();
	auto upper = example.toUpper();
	if (lower != lower_example)
	{
		LogError("toLower(\"%s\") returned \"%s\", expected \"%s\"", example, lower, lower_example);
		return EXIT_FAILURE;
	}
	if (upper != upper_example)
	{
		LogError("toUpper(\"%s\") returned \"%s\", expected \"%s\"", example, upper, upper_example);
		return EXIT_FAILURE;
	}

	UString removed_example1 = "€UPpa91£\"#ð𐍈";
	UString removed_example2 = "€UPpa91£B\"#ð";
	UString removed_example3 = "UPpa91£B\"#ð𐍈";
	UString removed_example4 = "€UPpa91£𐍈";
	UString empty = "";

	if (!test_remove(example, removed_example1, 8, 1))
		return EXIT_FAILURE;
	if (!test_remove(example, removed_example2, 12, 1))
		return EXIT_FAILURE;
	if (!test_remove(example, removed_example2, 12, 10))
		return EXIT_FAILURE;
	if (!test_remove(example, removed_example3, 0, 1))
		return EXIT_FAILURE;
	if (!test_remove(example, removed_example4, 8, 4))
		return EXIT_FAILURE;
	if (!test_remove(empty, empty, 8, 4))
		return EXIT_FAILURE;
	if (!test_remove(empty, empty, 0, 1))
		return EXIT_FAILURE;

	bool exception_caught = false;
	try
	{
		test_insert(empty, empty, 50, "Lol");
	}
	catch (const std::out_of_range &ex)
	{
		exception_caught = true;
	}
	if (!exception_caught)
	{
		LogError("\"\".insert(50 \"Lol\") didn't throw out_of_range exception");
		return EXIT_FAILURE;
	}

	UString insert_example1 = "Ayy€UPpa91£B\"#ð𐍈";
	UString insert_example2 = "€UPpaÞ91£B\"#ð𐍈";
	UString insert_example3 = "€UPpa91£B\"#ð𐍈€UPpa91£B\"#ð𐍈";

	test_insert(example, insert_example1, 0, "Ayy");
	test_insert(example, insert_example2, 5, "Þ");
	test_insert(example, insert_example3, 13, example);

	return EXIT_SUCCESS;
}
