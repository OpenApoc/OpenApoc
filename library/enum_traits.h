#pragma once

#include <type_traits>

namespace OpenApoc
{

// Trait for enums that don't define their domain completely
// enables (in)equality comparisons against underlying type
template <typename T> struct is_partial_enum : std::false_type
{
};

// Trait for enums that are meant to be used as flags
// enables bitwise operations against same type
template <typename T> struct is_flag_enum : std::false_type
{
};

// Equality comparisons
template <typename T>
typename std::enable_if<is_partial_enum<T>::value, bool>::type
operator==(const T &lhs, const typename std::underlying_type<T>::type &rhs)
{
	using UT = typename std::underlying_type<T>::type;
	return static_cast<UT>(lhs) == rhs;
}

template <typename T>
typename std::enable_if<is_partial_enum<T>::value, bool>::type
operator!=(const T &lhs, const typename std::underlying_type<T>::type &rhs)
{
	using UT = typename std::underlying_type<T>::type;
	return static_cast<UT>(lhs) != rhs;
}

template <typename T>
typename std::enable_if<is_partial_enum<T>::value, bool>::type
operator==(const typename std::underlying_type<T>::type &lhs, const T &rhs)
{
	return rhs == lhs;
}

template <typename T>
typename std::enable_if<is_partial_enum<T>::value, bool>::type
operator!=(const typename std::underlying_type<T>::type &lhs, const T &rhs)
{
	return rhs != lhs;
}

// Bitwise operations
template <typename T>
typename std::enable_if<is_flag_enum<T>::value, T &>::type operator&=(T &lhs, const T &rhs)
{
	using UT = typename std::underlying_type<T>::type;
	return lhs = static_cast<T>(static_cast<UT>(lhs) & static_cast<UT>(rhs));
}

template <typename T>
typename std::enable_if<is_flag_enum<T>::value, T &>::type operator|=(T &lhs, const T &rhs)
{
	using UT = typename std::underlying_type<T>::type;
	return lhs = static_cast<T>(static_cast<UT>(lhs) | static_cast<UT>(rhs));
}

template <typename T>
typename std::enable_if<is_flag_enum<T>::value, T &>::type operator^=(T &lhs, const T &rhs)
{
	using UT = typename std::underlying_type<T>::type;
	return lhs = static_cast<T>(static_cast<UT>(lhs) ^ static_cast<UT>(rhs));
}

template <typename T>
typename std::enable_if<is_flag_enum<T>::value, T>::type operator&(const T &lhs, const T &rhs)
{
	T copy = lhs;
	copy &= rhs;
	return copy;
}

template <typename T>
typename std::enable_if<is_flag_enum<T>::value, T>::type operator|(const T &lhs, const T &rhs)
{
	T copy = lhs;
	copy |= rhs;
	return copy;
}

template <typename T>
typename std::enable_if<is_flag_enum<T>::value, T>::type operator^(const T &lhs, const T &rhs)
{
	T copy = lhs;
	copy ^= rhs;
	return copy;
}

} // namespace OpenApoc
