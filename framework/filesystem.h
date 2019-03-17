#pragma once

#if defined(USE_BOOST_FILESYSTEM)

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#elif defined(USE_EXPERIMENTAL_FILESYSTEM)

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#else
#error No filesystem implementation
#endif
