#pragma once

#include "framework/configfile.h"
#include "framework/logger.h"
#include "game/state/gamestate.h"
#include "library/strings.h"
#include <cstdlib>
#include <utility>
#include <vector>

namespace OpenApoc
{
namespace TestHelpers
{

inline thread_local bool testCheckFailed = false;

#define TEST_CHECK(cond, ...)                                                                      \
	do                                                                                             \
	{                                                                                              \
		if (!(cond))                                                                               \
		{                                                                                          \
			LogError(__VA_ARGS__);                                                                 \
			::OpenApoc::TestHelpers::testCheckFailed = true;                                       \
		}                                                                                          \
	} while (0)

#define TEST_REQUIRE(cond, ...)                                                                    \
	do                                                                                             \
	{                                                                                              \
		if (!(cond))                                                                               \
		{                                                                                          \
			LogError(__VA_ARGS__);                                                                 \
			return false;                                                                          \
		}                                                                                          \
	} while (0)

inline void applyDeterministicTestConfig()
{
	config().set("OpenApoc.NewFeature.SeedRng", false);
	config().set("Config.Save", false);
	config().set("OpenApoc.NewFeature.FerryChecksRelationshipWhenBuying", true);
	config().set("OpenApoc.NewFeature.CallExistingFerry", true);
}

// NOTE: loadStartedGameState() lives with the gamestate work, not here. This branch carries only
// the harness, and the loader depends on extracted-table members that arrive with it.

inline int runTestSuite(const std::vector<std::pair<const char *, bool (*)()>> &tests)
{
	bool anyFailed = false;
	for (const auto &t : tests)
	{
		testCheckFailed = false;
		LogInfo("Running {0}", t.first);
		const bool ok = t.second();
		if (!ok || testCheckFailed)
		{
			LogError("FAILED {0}", t.first);
			anyFailed = true;
		}
		else
		{
			LogInfo("PASSED {0}", t.first);
		}
	}
	return anyFailed ? EXIT_FAILURE : EXIT_SUCCESS;
}

} // namespace TestHelpers
} // namespace OpenApoc
