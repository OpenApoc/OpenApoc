#include "library/strings.h"
#include "library/strings_format.h"
// Disable automatic #pragma linking for boost - only enabled in msvc and that should provide boost
// symbols as part of the module that uses it
#define BOOST_ALL_NO_LIB
#include <boost/algorithm/string/predicate.hpp>
#include <boost/locale.hpp>

#ifdef DUMP_TRANSLATION_STRINGS
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#endif

namespace OpenApoc
{

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

UString::UString(std::string str) : u8Str(str) {}

UString::UString(char c) : u8Str(1, c) {}

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
	u8Str = boost::locale::conv::utf_to_utf<char>(&uc, &uc + 1);
}

std::string UString::str() const { return this->u8Str; }

const char *UString::cStr() const { return this->u8Str.c_str(); }

bool UString::operator<(const UString &other) const { return (this->u8Str) < (other.u8Str); }

bool UString::operator==(const UString &other) const { return (this->u8Str) == (other.u8Str); }

UString UString::substr(size_t offset, size_t length) const
{
	return this->u8Str.substr(offset, length);
}

UString UString::toUpper() const { return boost::locale::to_upper(this->u8Str); }

UString UString::toLower() const { return boost::locale::to_lower(this->u8Str); }

UString &UString::operator=(const UString &other) = default;

UString &UString::operator+=(const UString &other)
{
	this->u8Str += other.u8Str;
	return *this;
}

size_t UString::length() const
{
	auto pointString = boost::locale::conv::utf_to_utf<UniChar>(this->u8Str);
	return pointString.length();
}

void UString::insert(size_t offset, const UString &other)
{
	this->u8Str.insert(offset, other.u8Str);
}

void UString::remove(size_t offset, size_t count) { this->u8Str.erase(offset, count); }

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
	return UString::ConstIterator(*this, this->length());
}

UString::ConstIterator UString::ConstIterator::operator++()
{
	this->offset++;
	return *this;
}

bool UString::ConstIterator::operator!=(const UString::ConstIterator &other) const
{
	return (this->offset != other.offset || this->s != other.s);
}

UniChar UString::ConstIterator::operator*() const
{
	auto pointString = boost::locale::conv::utf_to_utf<int>(this->s.str());
	return pointString[this->offset];
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
