
#pragma once

#include "framework/includes.h"

#include <unicode/unistr.h>

namespace OpenApoc {

typedef icu::UnicodeString UString;

UString U8Str(const char* str);
class Strings
{

	public:

		static std::vector<UString> Split(const UString &s, const UString &delims);
		static UString ToLower(const UString &s);
		static UString ToUpper(const UString &s);
		static int CompareCaseInsensitive(const UString &a, const UString &b);
		static bool IsNumeric(const UString &s);
		static int ToInteger(const UString &s);
		static uint8_t ToU8(const UString &s);
		static float ToFloat(const UString &s);
};
}; //namespace OpenApoc
