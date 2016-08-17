#pragma once

#include <cstdint>
#include <limits>
#include <ostream>
#include <istream>

namespace OpenApoc
{

template <class T, T A = 23, T B = 18, T C = 5> class xorshift_128_plus
{
  public:
	typedef T result_type;
	static_assert(std::is_unsigned<result_type>::value == true,
	              "xorshift_128_plus must have an unsigned type");

	xorshift_128_plus(result_type seed = 0)
	{
		// splitmix64 to initial seed to make sure there are some non-zero bits
		s[0] = static_cast<result_type>(splitmix_64(seed));
		s[1] = static_cast<result_type>(splitmix_64(seed | s[0]));
	}
	xorshift_128_plus(result_type state[2])
	{
		s[0] = state[0];
		s[1] = state[1];
	}

	void get_state(result_type out[]) const
	{
		out[0] = s[0];
		out[1] = s[1];
	}
	void set_state(result_type in[])
	{
		s[0] = in[0];
		s[1] = in[1];
	}

	result_type operator()()
	{
		result_type s1 = s[0];
		const result_type s0 = s[1];
		s[0] = s0;
		s1 ^= s1 << A;
		s[1] = s1 ^ s0 ^ (s1 >> B) ^ (s0 >> C);
		return s[1] + s0;
	}

#if _MSC_VER && _MSC_VER <= 1800
	static const result_type min() { return std::numeric_limits<result_type>::min(); }
	static const result_type max() { return std::numeric_limits<result_type>::max(); }
#else
	static constexpr result_type min() { return std::numeric_limits<result_type>::min(); }
	static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
#endif
  private:
	result_type s[2];
	static const uint64_t splitmix_64(uint64_t x)
	{
		uint64_t z = (x += static_cast<uint64_t>(0x9E3779B97F4A7C15));
		z = (z ^ (z >> 30)) * static_cast<uint64_t>(0xBF58476D1CE4E5B9);
		z = (z ^ (z >> 27)) * static_cast<uint64_t>(0x94D049BB133111EB);
		return z ^ (z >> 31);
	}
};

template <class charT, class traits, class UIntType>
std::basic_ostream<charT, traits> &operator<<(std::basic_ostream<charT, traits> &os,
                                              const xorshift_128_plus<UIntType> &rng)
{
	UIntType state[2];
	rng.get_state(state);
	return os << state[0] << " " << state[1];
}

template <class charT, class traits, class UIntType>
std::basic_istream<charT, traits> &operator>>(std::basic_istream<charT, traits> &is,
                                              xorshift_128_plus<UIntType> &rng)
{
	UIntType state[2];
	is >> state[0];
	is >> state[1];
	rng.set_state(state);
	return is;
}

template <typename T> using xorshift = xorshift_128_plus<T>;

} // namespace OpenApoc
