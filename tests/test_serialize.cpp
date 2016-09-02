#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize.h"
#include <boost/filesystem.hpp>

using namespace OpenApoc;

bool test_gamestate_serialization_roundtrip(sp<GameState> state, UString save_name)
{
	if (!state->saveGame(save_name))
	{
		LogError("Failed to save packed gamestate");
		return false;
	}

	auto read_gamestate = mksp<GameState>();
	if (!read_gamestate->loadGame(save_name))
	{
		LogError("Failed to load packed gamestate");
		return false;
	}

	if (*state != *read_gamestate)
	{
		LogError("Gamestate changed over serialization");
		return false;
	}
	return true;
}

bool test_gamestate_serialization(sp<GameState> state)
{
	auto tempPath = boost::filesystem::temp_directory_path() /
	                boost::filesystem::unique_path("openapoc_test_serialize-%%%%%%%%");
	UString pathString(tempPath.string());
	LogInfo("Writing temp state to \"%s\"", pathString.cStr());
	if (!test_gamestate_serialization_roundtrip(state, pathString))
	{
		LogError("Packed save test failed");
		return false;
	}

	boost::filesystem::remove(tempPath);

	return true;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		LogError("Unexpected args - expect ./test_serialize path/to/gamestate");
		return EXIT_FAILURE;
	}

	Framework fw("OpenApoc", {}, false);

	UString gamestate_name = argv[1];

	LogInfo("Loading \"%s\"", gamestate_name.cStr());

	auto state = mksp<GameState>();

	{
		auto state2 = mksp<GameState>();
		if (*state != *state2)
		{
			LogError("Empty gamestate failed comparison");
			return EXIT_FAILURE;
		}
	}

	if (!state->loadGame(gamestate_name))
	{
		LogError("Failed to load difficulty1_patched");
		return EXIT_FAILURE;
	}
	LogInfo("Testing non-started non-inited state");
	if (!test_gamestate_serialization(state))
	{
		LogError("Serialization test failed for non-started non-inited game");
		return EXIT_FAILURE;
	}

	LogInfo("Testing started non-inited state");
	state->startGame();

	if (!test_gamestate_serialization(state))
	{
		LogError("Serialization test failed for started non-inited game");
		return EXIT_FAILURE;
	}

	LogInfo("Testing started nited state");
	state->initState();

	if (!test_gamestate_serialization(state))
	{
		LogError("Serialization test failed for started inited game");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
