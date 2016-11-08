#pragma once

// Enable <experimental/filesystem> by default on MSVC, as we only support 2015+ anyway
#if !defined(USE_BOOST_FILESYSTEM) && !defined(USE_EXPERIMENTAL_FILESYSTEM) && defined (_MSC_VER)
#define USE_EXPERIMENTAL_FILESYSTEM
#endif


#if defined(USE_BOOST_FILESYSTEM)

#define BOOST_ALL_NO_LIB
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#elif defined(USE_EXPERIMENTAL_FILESYSTEM)

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#else
#error No filesystem implementation
#endif
