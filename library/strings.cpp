#include "library/strings.h"
#include "library/strings_format.h"
#include <cctype>
#include <tuple> // used for std::ignore
// Disable automatic #pragma linking for boost - only enabled in msvc and that should provide boost
// symbols as part of the module that uses it
#define BOOST_ALL_NO_LIB
#include <boost/algorithm/string/predicate.hpp>
#include <boost/locale/message.hpp>

#ifdef DUMP_TRANSLATION_STRINGS
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#endif

namespace OpenApoc
{

static constexpr UniChar REPLACEMENT_CHARACTER = 0xfffd;

static UniChar utf8_to_unichar(const char *str, size_t &num_bytes_read)
{
	/* This will pick up null bytes */
	if ((str[0] & 0b10000000) == 0)
	{
		num_bytes_read = 1;
		return static_cast<UniChar>(str[0]);
	}
	if ((str[0] & 0b11100000) == 0b11000000)
	{
		num_bytes_read = 2;
		if ((str[1] & 0b11000000) != 0b10000000)
		{
			return REPLACEMENT_CHARACTER;
		}
		UniChar c = 0;
		c = str[0] & 0b00011111;
		c <<= 6;
		c |= str[1] & 0b00111111;
		return c;
	}
	if ((str[0] & 0b11110000) == 0b11100000)
	{
		num_bytes_read = 3;
		if ((str[1] & 0b11000000) != 0b10000000 || (str[2] & 0b11000000) != 0b10000000)
		{
			return REPLACEMENT_CHARACTER;
		}
		UniChar c = 0;
		c = str[0] & 0b00001111;
		c <<= 6;
		c |= str[1] & 0b00111111;
		c <<= 6;
		c |= str[2] & 0b00111111;
		return c;
	}
	if ((str[0] & 0b11111000) == 0b11110000)
	{
		num_bytes_read = 4;
		if ((str[1] & 0b11000000) != 0b10000000 || (str[2] & 0b11000000) != 0b10000000 ||
		    (str[3] & 0b11000000) != 0b10000000)
		{
			return REPLACEMENT_CHARACTER;
		}
		UniChar c = 0;
		c = str[0] & 0b00000111;
		c <<= 6;
		c |= str[1] & 0b00111111;
		c <<= 6;
		c |= str[2] & 0b00111111;
		c <<= 6;
		c |= str[3] & 0b00111111;
		return c;
	}
	/* Invalid character start - return a replacement character and skip 1 char */
	num_bytes_read = 1;
	return REPLACEMENT_CHARACTER;
}

/* Returns the number of bytes used */
static size_t unichar_to_utf8(const UniChar uc, char str[4])
{
	if (uc < 0x7F)
	{
		str[0] = static_cast<char>(uc);
		return 1;
	}
	if (uc < 0x7FF)
	{
		UniChar c = uc;
		str[1] = 0b10000000 | (0b00111111 & c);
		c >>= 6;
		str[0] = 0b11000000 | (0b00011111 & c);
		return 2;
	}
	if (uc < 0xFFFF)
	{
		UniChar c = uc;
		str[2] = 0b10000000 | (0b00111111 & c);
		c >>= 6;
		str[1] = 0b10000000 | (0b00111111 & c);
		c >>= 6;
		str[0] = 0b11100000 | (0b00001111 & c);
		return 3;
	}
	if (uc < 0x10FFFF)
	{
		UniChar c = uc;
		str[3] = 0b10000000 | (0b00111111 & c);
		c >>= 6;
		str[2] = 0b10000000 | (0b00111111 & c);
		c >>= 6;
		str[1] = 0b10000000 | (0b00111111 & c);
		c >>= 6;
		str[0] = 0b11110000 | (0b00000111 & c);
		return 4;
	}
	return unichar_to_utf8(REPLACEMENT_CHARACTER, str);
}

#ifdef DUMP_TRANSLATION_STRINGS

static std::map<UString, std::set<UString>> trStrings;

void dumpStrings()
{
	for (auto &p : trStrings)
	{
		UString outFileName = p.first + ".po";
		std::ofstream outFile(outFileName.str());
		outFile << "msgid \"\"\n"
		        << "msgstr \"\"\n"
		        << "\"Project-Id-Version: Apocalypse\\n\"\n"
		        << "\"MIME-Version: 1.0\\n\"\n"
		        << "\"Content-Type: text/plain; charset=UTF-8\\n\"\n"
		        << "\"Content-Transfer-Encoding: 8bit\\n\"\n"
		        << "\"Language: en\\n\""
		        << "\"Plural-Forms: nplurals=2; plural=(n != 1);\\n\"\n\n";
		for (auto &str : p.second)
		{
			auto escapedStr = str;
			outFile << "msgid \"" << str << "\"\n";
			outFile << "msgstr \"" << str << "\"\n\n";
		}
	}
}

#endif

UString tr(const UString &str, const UString domain)
{
#ifdef DUMP_TRANSLATION_STRINGS
	if (str != "")
	{
		trStrings[domain].insert(str);
	}
#endif
	return UString(boost::locale::translate(str.str()).str(domain.str()));
}

UString::~UString() = default;

UString::UString() : u8Str() {}

UString::UString(const std::string &str) : u8Str(str) {}

UString::UString(std::string &&str) : u8Str(std::move(str)) {}

UString::UString(const char *cstr)

{
	// We have to handle this manually as some things thought UString(nullptr) was a good idea
	if (cstr)
	{
		this->u8Str = cstr;
	}
}

UString::UString(const UString &) = default;

UString::UString(UString &&other) { this->u8Str = std::move(other.u8Str); }

UString::UString(UniChar uc) : u8Str()
{
	char buf[4];
	auto bytes = unichar_to_utf8(uc, buf);
	u8Str = {buf, bytes};
}

const std::string &UString::str() const { return this->u8Str; }

const char *UString::cStr() const { return this->u8Str.c_str(); }

size_t UString::cStrLength() const { return this->u8Str.length(); }

bool UString::operator<(const UString &other) const { return (this->u8Str) < (other.u8Str); }

bool UString::operator==(const UString &other) const { return (this->u8Str) == (other.u8Str); }

UString UString::substr(size_t offset, size_t length) const
{
	return this->u8Str.substr(offset, length);
}

UString UString::toUpper() const
{
	/* Only change the case on ascii range characters (codepoint <=0x7f)
	 * As we know the top bit is set for any bytes outside this range no matter the postion in the
	 * utf8 stream, we can cheat a bit here */
	UString upper_string = *this;
	for (size_t i = 0; i < upper_string.cStrLength(); i++)
	{
		if ((upper_string.u8Str[i] & 0b10000000) == 0)
			upper_string.u8Str[i] = toupper(upper_string.u8Str[i]);
	}
	return upper_string;
}

UString UString::toLower() const
{
	/* Only change the case on ascii range characters (codepoint <=0x7f)
	 * As we know the top bit is set for any bytes outside this range no matter the postion in the
	 * utf8 stream, we can cheat a bit here */
	UString lower_string = *this;
	for (size_t i = 0; i < lower_string.cStrLength(); i++)
	{
		if ((lower_string.u8Str[i] & 0b10000000) == 0)
			lower_string.u8Str[i] = tolower(lower_string.u8Str[i]);
	}
	return lower_string;
}

UString &UString::operator=(const UString &other) = default;

UString &UString::operator+=(const UString &other)
{
	this->u8Str += other.u8Str;
	return *this;
}

size_t UString::length() const
{
	size_t len = 0;
	for (const auto &c : *this)
		len++;
	return len;
}

void UString::insert(size_t offset, const UString &other)
{
	auto it = this->begin();
	while (offset && it != this->end())
	{
		++it;
		offset--;
	}
	if (offset)
	{
		throw std::out_of_range("UString::insert() offset longer than string");
	}
	this->u8Str.insert(it.offset, other.u8Str);
}

void UString::remove(size_t offset, size_t count)
{
	auto it = this->begin();
	while (offset && it != this->end())
	{
		++it;
		offset--;
	}
	while (count && it != this->end())
	{
		size_t num_bytes;
		utf8_to_unichar(it.s.cStr() + it.offset, num_bytes);
		this->u8Str.erase(it.offset, num_bytes);
		count--;
	}
}

bool UString::operator!=(const UString &other) const { return this->u8Str != other.u8Str; }

UString operator+(const UString &lhs, const UString &rhs)
{
	UString s;
	s += lhs;
	s += rhs;
	return s;
}

std::ostream &operator<<(std::ostream &lhs, const UString &rhs)
{
	lhs << rhs.str();
	return lhs;
}

std::vector<UString> UString::split(const UString &delims) const
{
	// FIXME: Probably won't work if any of 'delims' is outside the ASCII range
	std::vector<UString> strings;
	size_t pos = 0;
	size_t prev = pos;
	while ((pos = this->u8Str.find_first_of(delims.str(), prev)) != std::string::npos)
	{
		if (pos > prev)
			strings.push_back(this->u8Str.substr(prev, pos - prev));
		prev = pos + 1;
	}
	strings.push_back(this->u8Str.substr(prev, pos));
	return strings;
}

std::list<UString> UString::splitlist(const UString &delims) const
{
	std::vector<UString> strings = split(delims);
	return std::list<UString>(strings.begin(), strings.end());
}

UniChar UString::u8Char(char c)
{
	// FIXME: I believe all the <256 codepoints just map?
	return c;
}

int UString::compare(const UString &other) const { return this->u8Str.compare(other.u8Str); }

bool UString::endsWith(const UString &suffix) const
{
	return boost::ends_with(str(), suffix.str());
}

UString::ConstIterator UString::begin() const { return UString::ConstIterator(*this, 0); }

UString::ConstIterator UString::end() const
{
	return UString::ConstIterator(*this, this->u8Str.length());
}

UString::ConstIterator UString::ConstIterator::operator++()
{
	const char *ptr = s.cStr();
	ptr += this->offset;
	size_t num_bytes = 0;
	utf8_to_unichar(ptr, num_bytes);

	this->offset += num_bytes;
	return *this;
}

bool UString::ConstIterator::operator!=(const UString::ConstIterator &other) const
{
	return (this->offset != other.offset || this->s != other.s);
}

UniChar UString::ConstIterator::operator*() const
{
	size_t num_bytes_unused;
	return utf8_to_unichar(this->s.cStr() + this->offset, num_bytes_unused);
}

int Strings::toInteger(const UString &s)
{
	std::string u8str = s.str();
	return static_cast<int>(strtol(u8str.c_str(), NULL, 0));
}

float Strings::toFloat(const UString &s)
{
	std::string u8str = s.str();
	return static_cast<float>(strtod(u8str.c_str(), NULL));
}

uint8_t Strings::toU8(const UString &s) { return static_cast<uint8_t>(toInteger(s)); }

bool Strings::isInteger(const UString &s)
{
	std::string u8str = s.str();
	char *endpos;
	std::ignore = strtol(u8str.c_str(), &endpos, 0);
	return (endpos != u8str.c_str());
}

bool Strings::isFloat(const UString &s)
{
	std::string u8str = s.str();
	char *endpos;
	std::ignore = strtod(u8str.c_str(), &endpos);
	return (endpos != u8str.c_str());
}

UString Strings::fromInteger(int i) { return format("%d", i); }

UString Strings::fromFloat(float f) { return format("%f", f); }

bool Strings::isWhiteSpace(UniChar c)
{
	// FIXME: Only works on ASCII whitespace
	return isspace(c) != 0;
}

UString Strings::fromU64(uint64_t i) { return format("%llu", i); }

}; // namespace OpenApoc
