#pragma once

#include <iterator>
#include <list>
#include <string>
#include <vector>

namespace OpenApoc
{

typedef char32_t UniChar;

class UString
{
  private:
	std::string u8Str;

  public:
	// ASSUMPTIONS:
	// All std::string/char are utf8
	// All lengths/offsets are in unicode code-points (not bytes/anything)
	UString(const std::string &str);
	UString(std::string &&str);
	UString(UniChar uc);
	UString(const char *cstr);
	UString(UString &&other);
	UString();
	~UString();

	UString(const UString &other);
	UString &operator=(const UString &other);

	const std::string &str() const;

	const char *cStr() const;
	size_t cStrLength() const;

	/* Only changes case of ASCII-range characters */
	UString toUpper() const;
	/* Only changes case of ASCII-range characters */
	UString toLower() const;
	std::vector<UString> split(const UString &delims) const;
	std::list<UString> splitlist(const UString &delims) const;

	size_t length() const;
	bool empty() const { return this->u8Str.empty(); }
	UString substr(size_t offset, size_t length = npos) const;

	static const size_t npos = static_cast<size_t>(-1);

	UString &operator+=(const UString &ustr);
	// UString& operator+=(const std::string& str);
	// UString& operator+=(const char* cstr);
	// UString& operator+=(const char& c);
	// UString& operator+=(const UniChar& uc);

	void remove(size_t offset, size_t count);
	void insert(size_t offset, const UString &other);

	int compare(const UString &str) const;

	bool endsWith(const UString &suffix) const;

	bool operator==(const UString &other) const;
	bool operator!=(const UString &other) const;
	bool operator<(const UString &other) const;

	class ConstIterator : public std::iterator<std::forward_iterator_tag, UniChar>
	{
	  private:
		const UString &s;
		size_t offset;
		friend class UString;
		ConstIterator(const UString &s, size_t initial_offset) : s(s), offset(initial_offset) {}

	  public:
		// Just enough to struggle through a range-based for
		bool operator!=(const ConstIterator &other) const;
		ConstIterator operator++();
		UniChar operator*() const;
	};
	ConstIterator begin() const;
	ConstIterator end() const;

	static UniChar u8Char(char c);
};

UString operator+(const UString &lhs, const UString &rhs);
std::ostream &operator<<(std::ostream &lhs, const UString &rhs);

class Strings
{

  public:
	static bool isFloat(const UString &s);
	static bool isInteger(const UString &s);
	static int toInteger(const UString &s);
	static uint8_t toU8(const UString &s);
	static float toFloat(const UString &s);
	static UString fromInteger(int i);
	static UString fromU64(uint64_t i);
	static UString fromFloat(float f);
	static bool isWhiteSpace(UniChar c);
};

#ifdef DUMP_TRANSLATION_STRINGS
void dumpStrings();
#endif

}; // namespace OpenApoc
