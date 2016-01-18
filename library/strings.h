
#pragma once

#include "framework/includes.h"
#include <iterator>
#include <limits>
#include <string>
#include <boost/format.hpp>
#include <boost/locale/format.hpp>

namespace OpenApoc
{

typedef char32_t UniChar;

class UString
{
  private:
	std::string u8Str;

	static boost::format &_format(boost::format &f)
	{
		return f;
	}

	template <typename T, typename... Args>
	static boost::format &_format(boost::format &f, T const &arg, Args &&...args)
	{
		return _format(f % arg, std::forward<Args>(args)...);
	}

	static boost::locale::format &_lformat(boost::locale::format &f)
	{
		return f;
	}

	template <typename T, typename... Args>
	static boost::locale::format &_lformat(boost::locale::format &f, T const &arg, Args &&...args)
	{
		return _lformat(f % arg, std::forward<Args>(args)...);
	}

  public:
	// ASSUMPTIONS:
	// All std::string/char are utf8
	// wchar_t/std::wstring are platform-dependant types
	// All lengths/offsets are in unicode code-points (not bytes/anything)
	UString(std::string str);
	UString(std::wstring wstr);
	UString(char c);
	UString(wchar_t wc);
	UString(UniChar uc);
	UString(const char *cstr);
	UString(const wchar_t *wcstr);
	UString(const UniChar *ucstr);
	UString(UString &&other);
	UString();
	~UString();

	UString(const UString &other);
	UString &operator=(const UString &other);

	template <typename... Args>
	static UString format(const UString &fmt, Args &&...args)
	{
		boost::format f(fmt.str());
		return _format(f, std::forward<Args>(args)...).str();
	}

	std::string str() const;
	std::wstring wstr() const;

	const char *c_str() const;

	UString toUpper() const;
	UString toLower() const;
	std::vector<UString> split(const UString &delims) const;

	size_t length() const;
	UString substr(size_t offset, size_t length = npos) const;

	static const size_t npos = static_cast<size_t>(-1);

	UString &operator+=(const UString &ustr);
	// UString& operator+=(const std::string& str);
	// UString& operator+=(const char* cstr);
	// UString& operator+=(const std::wstring& wstr);
	// UString& operator+=(const wchar_t* wcstr);
	// UString& operator+=(const char& c);
	// UString& operator+=(const wchar_t& wc);
	// UString& operator+=(const UniChar& uc);

	void remove(size_t offset, size_t count);
	void insert(size_t offset, const UString &other);

	int compare(const UString &str) const;

	bool operator==(const UString &other) const;
	bool operator!=(const UString &other) const;
	bool operator<(const UString &other) const;

	class const_iterator : public std::iterator<std::forward_iterator_tag, UniChar>
	{
	  private:
		const UString &s;
		size_t offset;
		friend class UString;
		const_iterator(const UString &s, size_t initial_offset) : s(s), offset(initial_offset) {}

	  public:
		// Just enough to struggle through a range-based for
		bool operator!=(const const_iterator &other) const;
		const_iterator operator++();
		UniChar operator*() const;
	};
	const_iterator begin() const;
	const_iterator end() const;

	static UniChar u8Char(char c);
};

UString operator+(const UString &lhs, const UString &rhs);

class Strings
{

  public:
	static bool IsFloat(const UString &s);
	static bool IsInteger(const UString &s);
	static int ToInteger(const UString &s);
	static uint8_t ToU8(const UString &s);
	static float ToFloat(const UString &s);
	static UString FromInteger(int i);
	static UString FromFloat(float f);
	static bool IsWhiteSpace(UniChar c);
};

UString tr(const UString &str, const UString domain = "ufo_string");

template <typename... Args>
static UString tr(const UString &fmt, const UString domain = "ufo_string", Args &&...args)
{
	boost::locale::format f(boost::locale::translate(fmt.str()).str(domain.str()));
	return _lformat(f, std::forward<Args>(args)...).str();
}

#ifdef DUMP_TRANSLATION_STRINGS
void dumpStrings();
#endif

}; // namespace OpenApoc
