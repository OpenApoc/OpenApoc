
#pragma once

#include "framework/includes.h"
#include <iterator>

#include <unicode/unistr.h>

namespace OpenApoc {


class UString2
{
private:
	class UString_impl;
	std::unique_ptr<UString_impl> pimpl;
public:
	//ASSUMPTIONS:
	//All std::string/char are utf8
	//wchar_t/std::wstring are platform-dependant types
	//All lengths/offsets are in unicode code-points (not bytes/anything)
	UString2(std::string str);
	UString2(std::wstring wstr);
	UString2(char c);
	UString2(wchar_t wc);
	UString2(UChar uc);
	~UString2();

	UString2(const UString2 &other);

	std::string str();
	std::wstring wstr();

	UString2 toUpper();
	UString2 toLower();
	std::vector<UString2> split(UString2 delims);

	size_t length();
	UString2 substr(size_t offset, size_t length = npos);

	const UChar& operator[](size_t pos) const;

	static const size_t npos = -1;

	UString2& operator+=(const UString2& ustr);
	UString2& operator+=(const std::string& str);
	UString2& operator+=(const char* cstr);
	UString2& operator+=(const std::wstring& wstr);
	UString2& operator+=(const wchar_t* wcstr);
	UString2& operator+=(const char& c);
	UString2& operator+=(const wchar_t& wc);
	UString2& operator+=(const UChar& uc);

	int compare(const UString2& str) const;

	bool operator==(const UString2& other);

	class const_iterator : public std::random_access_iterator_tag
	{
	public:
		//Just enough to struggle through a range-based for
		bool operator != (const const_iterator &other);
		const_iterator operator ++ ();
		const UChar & operator*();
	};
	const_iterator begin() const;
	const_iterator end() const;
};

UString2 operator+ (const UString2& lhs, const UString2& rhs);

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
};;
}; //namespace OpenApoc
