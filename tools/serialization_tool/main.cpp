#include "framework/configfile.h"
#include "framework/filesystem.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize.h"
#include <iostream>
#include <sstream>

using namespace OpenApoc;

static ConfigOptionString outputPath("", "output", "Path to write output archive to");
static ConfigOptionBool packOutput("", "pack", "Pack output into a zip instead of a directory",
                                   true);
static ConfigOptionBool prettyOutput("", "pretty", "Output more human-readable files (e.g. indent)",
                                     true);
static ConfigOptionString
    deltaGamestate("", "delta", "Only output the differences from specified parent gamestate");

int main(int argc, char **argv)
{
	config().addPositionalArgument("input1", "Input file");
	config().addPositionalArgument("input2", "Input file");
	config().addPositionalArgument("input3", "FIXME: Only 2 inputs supported right now");

	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	auto outputPath = config().getString("output");
	if (outputPath.empty())
	{
		std::cerr << "Must provide output path\n";
	}

	auto input1 = config().getString("input1");
	if (input1.empty())
	{
		std::cerr << "Must provide at least one input\n";
		config().showHelp();
		return EXIT_FAILURE;
	}
	auto input2 = config().getString("input2");
	auto input3 = config().getString("input3");
	if (!input3.empty())
	{
		std::cerr << "Only 2 inputs are supported right now\n";
		config().showHelp();
		return EXIT_FAILURE;
	}

	auto parentGamestate = config().getString("input1");
	auto pack = packOutput.get();
	auto pretty = prettyOutput.get();

	Framework fw("OpenApoc", false);

	auto state = mksp<GameState>();
	if (!state->loadGame(input1))
	{
		LogError("Failed to load input file \"%s\"", input1);
		return EXIT_FAILURE;
	}
	if (!input2.empty())
	{
		if (!state->loadGame(input2))
		{
			LogError("Failed to load second input file \"%s\"", input2);
			return EXIT_FAILURE;
		}
	}

	auto referenceState = mksp<GameState>();
	if (!parentGamestate.empty())
	{
		if (!referenceState->loadGame(parentGamestate))
		{
			LogError("Failed to load parent gamestate \"%s\"", parentGamestate);
			return EXIT_FAILURE;
		}
	}

	if (!state->saveGame(outputPath, pack, pretty))
	{
		LogError("Failed to write output gamestate to \"%s\"", outputPath);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
