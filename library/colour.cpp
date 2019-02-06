#include "colour.h"

#include <algorithm>
#include <array>
#include <map>

#include "framework/logger.h"

namespace OpenApoc
{

namespace
{

static const std::map<UString, Colour> html4Colours{
    {"white", {255, 255, 255}}, {"silver", {192, 192, 192}}, {"gray", {128, 128, 128}},
    {"black", {0, 0, 0}},       {"red", {255, 0, 0}},        {"maroon", {128, 0, 0}},
    {"yellow", {255, 255, 0}},  {"olive", {128, 128, 0}},    {"lime", {0, 255, 0}},
    {"green", {0, 128, 0}},     {"aqua", {0, 255, 255}},     {"teal", {0, 128, 128}},
    {"blue", {0, 0, 255}},      {"navy", {0, 0, 128}},       {"fuchsia", {255, 0, 255}},
    {"purple", {128, 0, 128}},
};
}

Colour Colour::FromHtmlName(const UString &name)
{
	auto it = html4Colours.find(name.toLower());
	if (it != html4Colours.end())
		return it->second;
	LogWarning("Unknown colour name: %s", name);
	return {0, 0, 0, 0};
}

Colour Colour::FromHex(const UString &hexcode)
{
	UString hexcode_lower = hexcode.toLower();
	if (hexcode_lower.empty())
	{
		LogWarning("Empty colour hex code");
		return {0, 0, 0};
	}
	// invalid initial character
	else if (*hexcode_lower.begin() != '#')
	{
		LogWarning("Unexpected hex triplet initial character: %c", *hexcode_lower.begin());
		return {0, 0, 0};
	}
	// invalid characters
	else if (!std::all_of(++hexcode_lower.begin(), hexcode_lower.end(), isxdigit))
	{
		LogWarning("Invalid characters found in hex triplet: %s", hexcode);
		return {0, 0, 0};
	}
	std::array<int, 6> digits;
	std::transform(++hexcode_lower.begin(), hexcode_lower.end(), digits.begin(),
	               [](int cp) { return '0' <= cp && cp <= '9' ? cp - '0' : cp - 'a' + 10; });
	if (hexcode_lower.length() == 4)
	{
		return {static_cast<uint8_t>(digits[0] * 0x11U), static_cast<uint8_t>(digits[1] * 0x11U),
		        static_cast<uint8_t>(digits[2] * 0x11U)};
	}
	else if (hexcode_lower.length() == 7)
	{
		return {static_cast<uint8_t>(digits[0] * 0x10U + digits[1]),
		        static_cast<uint8_t>(digits[2] * 0x10U + digits[3]),
		        static_cast<uint8_t>(digits[4] * 0x10U + digits[5])};
	}
	else
	{
		LogWarning("Unexpected hex triplet length (%d) in %s", hexcode_lower.length(), hexcode);
		return {0, 0, 0};
	}
}
}
