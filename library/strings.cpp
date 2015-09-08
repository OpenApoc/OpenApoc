#include "library/strings.h"

#include <unicode/unistr.h>
#include <unicode/umachine.h>
#include <unicode/uchar.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace OpenApoc
{

class UString::UString_impl : public icu::UnicodeString
{
  public:
	UString_impl(const std::string str) : icu::UnicodeString(str.c_str(), "UTF-8") {}
	UString_impl(const UChar *ucstr, int len) : icu::UnicodeString() { this->setTo(ucstr, len); }
	UString_impl(const char *str) : icu::UnicodeString(str, "UTF-8") {}
	UString_impl(const UString_impl &other) : icu::UnicodeString(other) {}
};

UString::~UString() {}

UString::UString() : pimpl(new UString_impl("")) {}

UString::UString(std::string str) : pimpl(new UString_impl(str)) {}

UString::UString(char c) : pimpl(new UString_impl(std::string(1, c))) {}

UString::UString(const char *cstr) : pimpl(new UString_impl(cstr)) {}

UString::UString(const UString &other) : pimpl(new UString_impl(*other.pimpl.get())) {}

UString::UString(UString &&other) { this->pimpl = std::move(other.pimpl); }

UString::UString(UniChar uc) : pimpl(new UString_impl("")) { pimpl->setTo((UChar32)uc); }

std::string UString::str() const
{
	std::string str;
	this->pimpl->toUTF8String(str);
	return str;
};

bool UString::operator<(const UString &other) const { return (*this->pimpl) < (*other.pimpl); }

bool UString::operator==(const UString &other) const { return (*this->pimpl) == (*other.pimpl); }

UString UString::substr(size_t offset, size_t length) const
{
	UString other;
	other.pimpl->setTo(*this->pimpl, offset, std::min(length, this->length()));
	return other;
}

UString UString::toUpper() const
{
	UString other;
	other.pimpl->setTo(this->pimpl->toUpper());
	return other;
}

UString &UString::operator=(const UString &other)
{
	this->pimpl->setTo(*other.pimpl);
	return *this;
}

UString &UString::operator+=(const UString &other)
{
	*this->pimpl += *other.pimpl;
	return *this;
}

UniChar UString::operator[](size_t pos) const { return this->pimpl->char32At(pos); }

size_t UString::length() const { return this->pimpl->countChar32(); }

void UString::insert(size_t offset, const UString &other)
{
	this->pimpl->insert(offset, *other.pimpl);
}

void UString::remove(size_t offset, size_t count) { this->pimpl->remove(offset, count); }

bool UString::operator!=(const UString &other) const { return *this->pimpl != *other.pimpl; }

UString operator+(const UString &lhs, const UString &rhs)
{
	UString s;
	s += lhs;
	s += rhs;
	return s;
}

std::vector<UString> UString::split(const UString &delims) const
{
	std::vector<UString> strings;
	int start = 0;
	int end = this->pimpl->indexOf(*delims.pimpl, start);
	while (end != -1)
	{
		strings.push_back(this->substr(start, (unsigned)(end - start)));
		start = end + 1;
		end = this->pimpl->indexOf(*delims.pimpl, start);
	}
	strings.push_back(this->substr(start, unsigned(end - start)));

	return strings;
}

UniChar UString::u8Char(char c)
{
	// FIXME: Evil nasty hack
	UString s(c);
	return s[0];
}

int UString::compare(const UString &other) const { return this->pimpl->compare(*other.pimpl); }

UString::const_iterator UString::begin() const { return UString::const_iterator(*this, 0); }

UString::const_iterator UString::end() const
{
	return UString::const_iterator(*this, this->length());
}

UString::const_iterator UString::const_iterator::operator++()
{
	this->offset++;
	return *this;
}

bool UString::const_iterator::operator!=(const UString::const_iterator &other) const
{
	return (this->offset != other.offset || this->s != other.s);
}

UniChar UString::const_iterator::operator*() const { return this->s[this->offset]; }

int Strings::ToInteger(const UString &s)
{
	std::string u8str = s.str();
	return (int)strtol(u8str.c_str(), NULL, 0);
}

float Strings::ToFloat(const UString &s)
{
	std::string u8str = s.str();
	return (float)strtod(u8str.c_str(), NULL);
}

uint8_t Strings::ToU8(const UString &s) { return (uint8_t)Strings::ToInteger(s); }

bool Strings::IsInteger(const UString &s)
{
	std::string u8str = s.str();
	char *endpos;
	std::ignore = strtol(u8str.c_str(), &endpos, 0);
	return (endpos != u8str.c_str());
}

bool Strings::IsFloat(const UString &s)
{
	std::string u8str = s.str();
	char *endpos;
	std::ignore = strtod(u8str.c_str(), &endpos);
	return (endpos != u8str.c_str());
}

UString Strings::FromInteger(int i)
{
	char buffer[50];
	snprintf(buffer, 50, "%d", i);
	return UString(buffer);
}

UString Strings::FromFloat(float f)
{
	char buffer[50];
	snprintf(buffer, 50, "%f", f);
	return UString(buffer);
}

bool Strings::IsWhiteSpace(UniChar c) { return u_isWhitespace(c); }

}; // namespace OpenApoc
