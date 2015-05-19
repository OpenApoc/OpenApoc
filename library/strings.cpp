#include "library/strings.h"

namespace OpenApoc
{

	UString U8Str(const char* str)
	{
		return icu::UnicodeString::fromUTF8(icu::StringPiece(str));
	}

	std::vector<UString>
	Strings::Split(const UString &s, const UString &delims)
	{
		std::vector<UString> strings;
		strings.push_back("");
		for (int i = 0; i < s.length(); i++)
		{
			bool delim = false;
			for (int j = 0; j < delims.length(); j++)
			{
				if (s.charAt(i) == delims.charAt(j))
				{
					strings.push_back("");
					delim = true;
					break;
				}
			}
			if (!delim)
				strings.back().append(s.charAt(i));
		}
		return strings;
	}

	UString
	Strings::ToLower(const UString &s)
	{
		UString ls = s;
		ls.toLower();
		return ls;
	}

	UString
	Strings::ToUpper(const UString &s)
	{
		UString us = s;
		us.toUpper();
		return us;
	}

	int
	Strings::ToInteger(const UString &s)
	{
		//FIXME: Hack - this relies on utf8 numerals being the same as ascii
		std::string U8Str;
		s.toUTF8String(U8Str);
		return strtol(U8Str.c_str(), NULL, 0);
	}
	uint8_t
	Strings::ToU8(const UString &s)
	{
		return (uint8_t)ToInteger(s);
	}

	bool
	Strings::IsNumeric(const UString &s)
	{
		//FIXME: Hack - this relies on utf8 numerals being the same as ascii
		std::string U8Str;
		s.toUTF8String(U8Str);
		char *endptr;
		strtol(U8Str.c_str(), &endptr, 0);
		//If the endptr is not the start of the string some numbers were successfully parsed
		return (endptr != U8Str.c_str());
		
	}

}; //namespace OpenApoc
