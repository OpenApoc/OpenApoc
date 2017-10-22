#pragma once

#include <cstdint>
#include <istream>
#include <limits>
#include <list>
#include <map>
#include <ostream>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>

namespace OpenApoc
{

template <class T, uint64_t A = 23, uint64_t B = 18, uint64_t C = 5> class Xorshift128Plus
{
  public:
	typedef T result_type;
	static_assert(std::is_unsigned<result_type>::value == true,
	              "Xorshift128Plus must have an unsigned type");
	static_assert(std::numeric_limits<result_type>::max() <= std::numeric_limits<uint64_t>::max(),
	              "Can only be used on types of uint64_t size or smaller");

	Xorshift128Plus(uint64_t seed = 0)
	{
		// splitmix64 to initial seed to make sure there are some non-zero bits
		s[0] = static_cast<result_type>(splitmix64(seed));
		s[1] = static_cast<result_type>(splitmix64(seed ^ s[0]));
	}
	Xorshift128Plus(uint64_t state[2])
	{
		s[0] = state[0];
		s[1] = state[1];
	}

	void getState(uint64_t out[]) const
	{
		out[0] = s[0];
		out[1] = s[1];
	}
	void setState(uint64_t in[])
	{
		s[0] = in[0];
		s[1] = in[1];
	}

	result_type operator()()
	{
		uint64_t s1 = s[0];
		const uint64_t s0 = s[1];
		s[0] = s0;
		s1 ^= s1 << A;
		s[1] = s1 ^ s0 ^ (s1 >> B) ^ (s0 >> C);
		return static_cast<result_type>(s[1] + s0);
	}

	bool operator==(const Xorshift128Plus<T, A, B, C> &other) const
	{
		return (this->s[0] == other.s[0] && this->s[1] == other.s[1]);
	}
	bool operator!=(const Xorshift128Plus<T, A, B, C> &other) const { return !(*this == other); }

#if _MSC_VER && _MSC_VER <= 1800
	static const result_type min() { return std::numeric_limits<result_type>::min(); }
	static const result_type max() { return std::numeric_limits<result_type>::max(); }
#else
	static constexpr result_type min() { return std::numeric_limits<result_type>::min(); }
	static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
#endif
  private:
	uint64_t s[2];
	static uint64_t splitmix64(uint64_t x)
	{
		uint64_t z = (x += static_cast<uint64_t>(0x9E3779B97F4A7C15));
		z = (z ^ (z >> 30)) * static_cast<uint64_t>(0xBF58476D1CE4E5B9);
		z = (z ^ (z >> 27)) * static_cast<uint64_t>(0x94D049BB133111EB);
		return z ^ (z >> 31);
	}
};

template <class charT, class traits, class UIntType>
std::basic_ostream<charT, traits> &operator<<(std::basic_ostream<charT, traits> &os,
                                              const Xorshift128Plus<UIntType> &rng)
{
	uint64_t state[2];
	rng.get_state(state);
	return os << state[0] << " " << state[1];
}

template <class charT, class traits, class UIntType>
std::basic_istream<charT, traits> &operator>>(std::basic_istream<charT, traits> &is,
                                              Xorshift128Plus<UIntType> &rng)
{
	uint64_t state[2];
	is >> state[0];
	is >> state[1];
	rng.set_state(state);
	return is;
}

template <typename T> using xorshift = Xorshift128Plus<T>;

template <typename T, typename Generator>
T probabilityMapRandomizer(Generator &g, const std::map<T, float> &probabilityMap)
{
	if (probabilityMap.empty())
	{
		throw std::runtime_error("Called with empty probabilityMap");
	}
	float total = 0.0f;
	for (auto &p : probabilityMap)
	{
		total += p.second;
	}
	std::uniform_real_distribution<float> dist(0, total);

	float val = dist(g);

	// Due to fp precision there's a small chance the total will be slightly more than the max,
	// so have a fallback just in case?
	T fallback = probabilityMap.begin()->first;
	total = 0.0f;

	for (auto &p : probabilityMap)
	{
		if (val < total + p.second)
		{
			return p.first;
		}
		total += p.second;
	}
	return fallback;
}

template <typename T, typename Generator> T randBoundsExclusive(Generator &g, T min, T max)
{
	return randBoundsInclusive(g, min, max - 1);
}

template <typename Generator> bool randBool(Generator &g)
{
	return randBoundsInclusive(g, 0, 1) == 0;
}

template <typename T, typename Generator> T randBoundsInclusive(Generator &g, T min, T max)
{
	if (min > max)
	{
		throw std::runtime_error("Bounds max < min");
	}
	// uniform_int_distribution is apparently undefined if min==max
	if (min == max)
		return min;
	std::uniform_int_distribution<T> dist(min, max);
	return dist(g);
}

template <typename T, typename Generator> T randDamage000200(Generator &g, T value)
{
	T min = 0;
	T max = value * (T)2;
	if (min > max)
	{
		throw std::runtime_error("Bounds max < min");
	}
	// uniform_int_distribution is apparently undefined if min==max
	if (min == max)
		return min;
	std::uniform_int_distribution<T> dist(min, max);
	return dist(g);
}

template <typename T, typename Generator> T randDamage050150(Generator &g, T value)
{
	T min = value / (T)2;
	T max = value * (T)3 / (T)2;
	if (min > max)
	{
		throw std::runtime_error("Bounds max < min");
	}
	// uniform_int_distribution is apparently undefined if min==max
	if (min == max)
		return min;
	std::uniform_int_distribution<T> dist(min, max);
	return dist(g);
}

template <typename T, typename Generator> T randDamage025075(Generator &g, T value)
{
	T min = value / (T)4;
	T max = value * (T)3 / (T)4;
	if (min > max)
	{
		throw std::runtime_error("Bounds max < min");
	}
	// uniform_int_distribution is apparently undefined if min==max
	if (min == max)
		return min;
	std::uniform_int_distribution<T> dist(min, max);
	return dist(g);
}
template <typename T, typename Generator> T listRandomiser(Generator &g, const std::list<T> &list)
{
	// we can't do index lookups in a list, so we just have to iterate N times
	if (list.size() == 1)
		return *list.begin();
	else if (list.empty())
	{
		throw std::runtime_error("Trying to randomize within empty list");
	}
	auto count = randBoundsExclusive(g, (unsigned)0, (unsigned)list.size());

	auto it = list.begin();
	while (count)
	{
		it++;
		count--;
	}
	return *it;
}

template <typename T, typename Generator> T setRandomiser(Generator &g, const std::set<T> &set)
{
	// we can't do index lookups in a set, so we just have to iterate N times
	if (set.size() == 1)
		return *set.begin();
	else if (set.empty())
	{
		throw std::runtime_error("Trying to randomize within empty list");
	}
	auto count = randBoundsExclusive(g, (unsigned)0, (unsigned)set.size());

	auto it = set.begin();
	while (count)
	{
		it++;
		count--;
	}
	return *it;
}

template <typename T1, typename T2, typename Generator>
std::pair<T1, T2> mapRandomizer(Generator &g, const std::map<T1, T2> &map)
{
	// we can't do index lookups in a set, so we just have to iterate N times
	if (map.size() == 1)
		return *map.begin();
	else if (map.empty())
	{
		throw std::runtime_error("Trying to randomize within empty map");
	}
	auto count = randBoundsExclusive(g, (unsigned)0, (unsigned)map.size());

	auto it = map.begin();
	while (count)
	{
		it++;
		count--;
	}
	return *it;
}

template <typename T, typename Generator>
T vectorRandomizer(Generator &g, const std::vector<T> &vector)
{
	// we can't do index lookups in a list, so we just have to iterate N times
	if (vector.size() == 1)
		return *vector.begin();
	else if (vector.empty())
	{
		throw std::runtime_error("Trying to randomize within empty vector");
	}
	auto count = randBoundsExclusive(g, (unsigned)0, (unsigned)vector.size());
	auto it = vector.begin();
	while (count)
	{
		it++;
		count--;
	}
	return *it;
}

template <typename T, typename Generator> T setRandomizer(Generator &g, const std::set<T> &set)
{
	// we can't do index lookups in a list, so we just have to iterate N times
	if (set.size() == 1)
		return *set.begin();
	else if (set.empty())
	{
		throw std::runtime_error("Trying to randomize within empty vector");
	}
	auto count = randBoundsExclusive(g, (unsigned)0, (unsigned)set.size());
	auto it = set.begin();
	while (count)
	{
		it++;
		count--;
	}
	return *it;
}

} // namespace OpenApoc
