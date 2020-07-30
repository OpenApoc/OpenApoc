#include "framework/configfile.h"
#include "framework/logger.h"
#include "library/strings.h"

using namespace OpenApoc;

static bool test_32_8_roundtrip(const UString &str)
{
	auto u32str = to_u32string(str);
	auto u8str = to_ustring(u32str);
	if (u8str != str)
	{
		LogError("String \"%s\" ended up as \"%s\" after utf8->utf32->utf8 roundtrip", str, u8str);
		return false;
	}
	return true;
}

struct example_unicode
{
	const char *u8string;
	std::vector<char32_t> expected_codepoints;

	example_unicode(const char *str, std::vector<char32_t> codepoints)
	    : u8string(str), expected_codepoints(codepoints){};
#ifdef __cpp_char8_t
	example_unicode(const char8_t *str, std::vector<UniChar> codepoints)
	    : u8string(reinterpret_cast<const char *>(str)), expected_codepoints(codepoints){};
#endif

	bool test() const
	{
		LogInfo("Testing string \"%s\"", u8string);
		const auto num_codepoints = expected_codepoints.size();
		UString string(u8string);
		U32String string2;
		auto u32str = to_u32string(string);
		if (u32str.length() != num_codepoints)
		{
			LogError("String \"%s\" has unexpected length %zu , expected %zu", u8string,
			         string.length(), num_codepoints);
			return false;
		}

		std::vector<char32_t> decoded_codepoints;

		for (auto c : u32str)
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
				    u8string, i, static_cast<uint32_t>(decoded_codepoints[i]),
				    static_cast<uint32_t>(expected_codepoints[i]));
				return false;
			}
			string2 += decoded_codepoints[i];
		}

		if (string != to_ustring(string2))
		{
			LogError(
			    "String \"%s\" compared false after utf8->unichar->utf8 conversion - got \"%s\"",
			    u8string, to_ustring(string2));

			return false;
		}

		if (!test_32_8_roundtrip(u8string))
		{
			return false;
		}

		LogInfo("Looks good");

		return true;
	}
};

static bool test_remove(const UString &initial, const UString &expected, size_t offset,
                        size_t count)
{
	auto removed = remove(initial, offset, count);
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
	auto inserted = insert_codepoints(initial, offset, str);
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

	UString example = u8"€UPpa91£B\"#ð𐍈";
	UString lower_example = u8"€uppa91£b\"#ð𐍈";
	UString upper_example = u8"€UPPA91£B\"#ð𐍈";

	auto lower = to_lower(example);
	auto upper = to_upper(example);
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

	UString removed_example1 = u8"€UPpa91£\"#ð𐍈";
	UString removed_example2 = u8"€UPpa91£B\"#ð";
	UString removed_example3 = u8"UPpa91£B\"#ð𐍈";
	UString removed_example4 = u8"€UPpa91£𐍈";
	UString empty = u8"";

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

	bool exception_caught = false;
	try
	{
		test_insert(empty, empty, 50, u8"Lol");
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

	UString insert_example1 = u8"Ayy€UPpa91£B\"#ð𐍈";
	UString insert_example2 = u8"€UPpaÞ91£B\"#ð𐍈";
	UString insert_example3 = u8"€UPpa91£B\"#ð𐍈€UPpa91£B\"#ð𐍈";

	test_insert(example, insert_example1, 0, u8"Ayy");
	test_insert(example, insert_example2, 5, u8"Þ");
	test_insert(example, insert_example3, 13, example);

	test_32_8_roundtrip(removed_example1);
	test_32_8_roundtrip(removed_example2);
	test_32_8_roundtrip(removed_example3);
	test_32_8_roundtrip(removed_example4);
	test_32_8_roundtrip(insert_example1);
	test_32_8_roundtrip(insert_example2);
	test_32_8_roundtrip(insert_example3);

	return EXIT_SUCCESS;
}
