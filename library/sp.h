#pragma once

#include <memory>

/* A simple alias for sp<T> because I'm wearing out my keyboard */

namespace OpenApoc
{

template <class T> using sp = std::shared_ptr<T>;
template <class T> using wp = std::weak_ptr<T>;
template <class T> using up = std::unique_ptr<T>;

template <typename T, typename... Args> sp<T> mksp(Args &&...args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

/* make_unique was only added in c++14 (probably an oversight) */
#ifdef __cpp_lib_make_unique
template <typename T, typename... Args> up<T> mkup(Args &&...args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}
#else
template <typename T, typename... Args> up<T> mkup(Args &&...args)
{
	return up<T>(new T(std::forward<Args>(args)...));
}
#endif

} // namespace OpenApoc
