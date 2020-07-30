#include "library/strings.h"
#include "library/strings_format.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/locale/encoding_utf.hpp>
#include <boost/locale/message.hpp>
#include <cctype>

namespace OpenApoc
{

UString tr(const UString &str, const UString domain)
{
	return UString(boost::locale::translate(str).str(domain));
}

U32String to_u32string(const UStringView str)
{
	// FIXME: Boost api doesn't work with string views yet?
	auto string_copy = UString(str);
	return boost::locale::conv::utf_to_utf<char32_t>(string_copy);
}

UString to_ustring(const std::u32string_view str)
{
	auto string_copy = U32String(str);
	return boost::locale::conv::utf_to_utf<char>(string_copy);
}

char32_t to_char32(const char c)
{
	// All the codepoints <=127 are the same as ascii
	return static_cast<char32_t>(c);
}

UString to_upper(const UStringView str)
{
	/* Only change the case on ascii range characters (codepoint <=0x7f)
	 * As we know the top bit is set for any bytes outside this range no matter the position in the
	 * utf8 stream, we can cheat a bit here */
	auto upper_string = UString(str);
	for (size_t i = 0; i < upper_string.length(); i++)
	{
		if ((upper_string[i] & 0b10000000) == 0)
			upper_string[i] = toupper(upper_string[i]);
	}
	return upper_string;
}

UString to_lower(const UStringView str)
{
	/* Only change the case on ascii range characters (codepoint <=0x7f)
	 * As we know the top bit is set for any bytes outside this range no matter the position in the
	 * utf8 stream, we can cheat a bit here */
	auto lower_string = UString(str);
	for (size_t i = 0; i < lower_string.length(); i++)
	{
		if ((lower_string[i] & 0b10000000) == 0)
			lower_string[i] = tolower(lower_string[i]);
	}
	return lower_string;
}

bool ends_with(const UStringView str, const UStringView ending)
{
	return boost::ends_with(str, ending);
}

UString remove(const UStringView str, size_t offset, size_t count)
{
	auto u32str = to_u32string(str);

	u32str.erase(offset, count);

	return to_ustring(u32str);
}

U32String remove(const U32StringView str, size_t offset, size_t count)
{
	auto str_copy = U32String(str);
	return str_copy.erase(offset, count);
}

std::vector<UString> split(const UStringView str, const UStringView delims)
{
	// FIXME: Probably won't work if any of 'delims' is outside the ASCII range
	std::vector<UString> strings;
	size_t pos = 0;
	size_t prev = pos;
	while ((pos = str.find_first_of(delims, prev)) != std::string::npos)
	{
		if (pos > prev)
			strings.push_back(UString(str.substr(prev, pos - prev)));
		prev = pos + 1;
	}
	strings.push_back(UString(str.substr(prev, pos)));
	return strings;
}

UString insert_codepoints(const UStringView str, size_t offset, const UStringView insert)
{
	auto u32str = to_u32string(str);
	auto u32insert = to_u32string(insert);
	u32str.insert(offset, u32insert);
	return to_ustring(u32str);
}

int Strings::toInteger(const UStringView s)
{
	auto u8str = UString(s);
	return static_cast<int>(strtol(u8str.c_str(), NULL, 0));
}

float Strings::toFloat(const UStringView s)
{
	auto u8str = UString(s);
	return static_cast<float>(strtod(u8str.c_str(), NULL));
}

uint8_t Strings::toU8(const UStringView s) { return static_cast<uint8_t>(toInteger(s)); }

bool Strings::isInteger(const UStringView s)
{
	auto u8str = UString(s);
	char *endpos;
	std::ignore = strtol(u8str.c_str(), &endpos, 0);
	return (endpos != u8str.c_str());
}

bool Strings::isFloat(const UStringView s)
{
	auto u8str = UString(s);
	char *endpos;
	std::ignore = strtod(u8str.c_str(), &endpos);
	return (endpos != u8str.c_str());
}

UString Strings::fromInteger(int i) { return format("%d", i); }

UString Strings::fromFloat(float f) { return format("%f", f); }

bool Strings::isWhiteSpace(char32_t c)
{
	// FIXME: Only works on ASCII whitespace
	return isspace(c) != 0;
}

UString Strings::fromU64(uint64_t i) { return format("%llu", i); }

}; // namespace OpenApoc
