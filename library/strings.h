#pragma once

#include <iterator>
#include <list>
#include <string>
#include <string_view>
#include <vector>

namespace OpenApoc
{

//#ifdef __cpp_char8_t__not_in_this_castle
//#warning Using char8_t
// using UString = std::basic_string<char8_t>;
// using ustring_view = std::basic_string_view<char8_t>;
//#else
//#warning Using char
using UString = std::basic_string<char>;
using UStringView = std::basic_string_view<char>;
//#endif

using U32String = std::basic_string<char32_t>;
using U32StringView = std::basic_string_view<char32_t>;

[[nodiscard]] U32String to_u32string(const UStringView str);
[[nodiscard]] UString to_ustring(const std::u32string_view str);
[[nodiscard]] char32_t to_char32(const char c);

[[nodiscard]] UString to_lower(const UStringView str);
[[nodiscard]] UString to_upper(const UStringView str);

[[nodiscard]] bool ends_with(const UStringView str, const UStringView ending);

// Removes 'count' codepoints at 'offset' codepoints into the string (note: Not bytes!)
[[nodiscard]] UString remove(const UStringView str, size_t offset, size_t count);
[[nodiscard]] U32String remove(const U32StringView str, size_t offset, size_t count);

[[nodiscard]] std::vector<UString> split(const UStringView str, const UStringView delims);

// Insert the 'insert' string at 'offset' codepoints into the string 'str' and returns the string
[[nodiscard]] UString insert_codepoints(const UStringView str, size_t offset,
                                        const UStringView insert);

class Strings
{

  public:
	[[nodiscard]] static bool isFloat(const UStringView s);
	[[nodiscard]] static bool isInteger(const UStringView s);
	[[nodiscard]] static int toInteger(const UStringView s);
	[[nodiscard]] static uint8_t toU8(const UStringView s);
	[[nodiscard]] static float toFloat(const UStringView s);
	[[nodiscard]] static UString fromInteger(int i);
	[[nodiscard]] static UString fromU64(uint64_t i);
	[[nodiscard]] static UString fromFloat(float f);
	[[nodiscard]] static bool isWhiteSpace(char32_t c);
};

}; // namespace OpenApoc
