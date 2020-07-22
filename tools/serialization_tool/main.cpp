#include "framework/configfile.h"
#include "framework/filesystem.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize.h"
#include <iostream>
#include <sstream>

// We can't just use 'using namespace OpenApoc;' as:
// On windows VS it says
/* Error	C2678	binary '==': no operator found which takes a left - hand operand of type
'Concurrency::details::_Task_impl<_ReturnType>'
(or there is no acceptable conversion)	test_serialize	E :
\Projects\GitHub\OpenApoc\game\state\gamestate_serialize.h	106

My (JonnyH) assumption is that some part of the system library is defining some operator== for
std::shared_ptr<SomeInternalConcurrencyType>, but it resolves to our OpenApoc operator== instead.
*/

static OpenApoc::ConfigOptionString outputPath("", "output", "Path to write output archive to");
static OpenApoc::ConfigOptionBool packOutput("", "pack",
                                             "Pack output into a zip instead of a directory", true);
static OpenApoc::ConfigOptionBool
    prettyOutput("", "pretty", "Output more human-readable files (e.g. indent)", true);
static OpenApoc::ConfigOptionString
    deltaGamestate("", "delta", "Only output the differences from specified parent gamestate");

int main(int argc, char **argv)
{
	OpenApoc::config().addPositionalArgument("input1", "Input file");
	OpenApoc::config().addPositionalArgument("input2", "Input file");
	OpenApoc::config().addPositionalArgument("input3", "FIXME: Only 2 inputs supported right now");

	if (OpenApoc::config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	auto outputPath = OpenApoc::config().getString("output");
	if (outputPath.empty())
	{
		std::cerr << "Must provide output path\n";
	}

	auto input1 = OpenApoc::config().getString("input1");
	if (input1.empty())
	{
		std::cerr << "Must provide at least one input\n";
		OpenApoc::config().showHelp();
		return EXIT_FAILURE;
	}
	auto input2 = OpenApoc::config().getString("input2");
	auto input3 = OpenApoc::config().getString("input3");
	if (!input3.empty())
	{
		std::cerr << "Only 2 inputs are supported right now\n";
		OpenApoc::config().showHelp();
		return EXIT_FAILURE;
	}

	auto parentGamestate = OpenApoc::config().getString("input1");
	auto pack = packOutput.get();
	auto pretty = prettyOutput.get();

	OpenApoc::Framework fw("OpenApoc", false);

	auto state = OpenApoc::mksp<OpenApoc::GameState>();
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

	auto referenceState = OpenApoc::mksp<OpenApoc::GameState>();
	auto deltaPath = deltaGamestate.get();
	if (!deltaPath.empty())
	{
		if (!referenceState->loadGame(deltaPath))
		{
			LogError("Failed to load delta gamestate \"%s\"", parentGamestate);
			return EXIT_FAILURE;
		}
	}

	if (!state->saveGameDelta(outputPath, *referenceState, pack, pretty))
	{
		LogError("Failed to write output gamestate to \"%s\"", outputPath);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
