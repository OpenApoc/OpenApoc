#pragma once

#include <memory>

/* A simple alias for sp<T> because I'm wearing out my keyboard */

namespace OpenApoc
{

template <class T> using sp = std::shared_ptr<T>;
template <class T> using wp = std::weak_ptr<T>;
template <class T> using up = std::unique_ptr<T>;

template <typename T, typename... Args> sp<T> mksp(Args &&... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

} // namespace OpenApoc
