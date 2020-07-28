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

bool test_gamestate_serialization_roundtrip(OpenApoc::sp<OpenApoc::GameState> state,
                                            OpenApoc::UString save_name)
{
	if (!state->saveGame(save_name))
	{

		LogWarning("Failed to save packed gamestate");
		return false;
	}

	auto read_gamestate = OpenApoc::mksp<OpenApoc::GameState>();
	if (!read_gamestate->loadGame(save_name))
	{
		LogWarning("Failed to load packed gamestate");
		return false;
	}

#if 0
	// FIXME: This isn't reliable due to undefined order of containers
	if (*state != *read_gamestate)
#endif
	if (0)
	{
		LogWarning("Gamestate changed over serialization");

		return false;
	}
	return true;
}

bool test_gamestate_serialization(OpenApoc::sp<OpenApoc::GameState> state)
{

	std::stringstream ss;
	ss << "openapoc_test_serialize-" << std::this_thread::get_id();
	auto tempPath = fs::temp_directory_path() / ss.str();
	OpenApoc::UString pathString(tempPath.string());
	LogInfo("Writing temp state to \"%s\"", pathString);
	if (!test_gamestate_serialization_roundtrip(state, pathString))
	{
		LogWarning("Packed save test failed");
		return false;
	}

	fs::remove(tempPath);

	return true;
}

int main(int argc, char **argv)
{
	OpenApoc::config().addPositionalArgument("common", "Common gamestate to load");
	OpenApoc::config().addPositionalArgument("gamestate", "Gamestate to load");

	if (OpenApoc::config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	auto gamestate_name = OpenApoc::config().getString("gamestate");
	if (gamestate_name.empty())
	{
		std::cerr << "Must provide gamestate\n";
		OpenApoc::config().showHelp();
		return EXIT_FAILURE;
	}
	auto common_name = OpenApoc::config().getString("common");
	if (common_name.empty())
	{
		std::cerr << "Must provide common gamestate\n";
		OpenApoc::config().showHelp();
		return EXIT_FAILURE;
	}

	OpenApoc::Framework fw("OpenApoc", false);

	LogInfo("Loading \"%s\"", gamestate_name);

	auto state = OpenApoc::mksp<OpenApoc::GameState>();

	{
		auto state2 = OpenApoc::mksp<OpenApoc::GameState>();
		if (*state != *state2)
		{
			LogError("Empty gamestate failed comparison");
			return EXIT_FAILURE;
		}
	}
	if (!state->loadGame(common_name))
	{
		LogError("Failed to load gamestate_common");
		return EXIT_FAILURE;
	}

	if (!state->loadGame(gamestate_name))
	{
		LogError("Failed to load supplied gamestate");
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

	LogInfo("Testing started inited state");
	state->initState();
	state->fillPlayerStartingProperty();
	state->fillOrgStartingProperty();

	if (!test_gamestate_serialization(state))
	{
		LogError("Serialization test failed for started inited game");
		return EXIT_FAILURE;
	}

	LogInfo("Testing state with battle");
	LogInfo("--Test disabled until we find a way to compare sets properly (fails in sets of "
	        "pointers like hazards)--");
	if (false)
	{

		OpenApoc::StateRef<OpenApoc::Organisation> org = {state.get(),
		                                                  OpenApoc::UString("ORG_ALIEN")};
		auto v = OpenApoc::mksp<OpenApoc::Vehicle>();
		auto vID = OpenApoc::Vehicle::generateObjectID(*state);
		OpenApoc::sp<OpenApoc::VehicleType> vType;

		// Fine a vehicle type with a battlemap
		for (auto &vTypePair : state->vehicle_types)
		{
			if (vTypePair.second->battle_map)
			{
				vType = vTypePair.second;
				break;
			}
		}
		if (!vType)
		{
			LogError("No vehicle with BattleMap found");
			return EXIT_FAILURE;
		}
		LogInfo("Using vehicle map for \"%s\"", vType->name);
		v->type = {state.get(), vType};
		v->name = OpenApoc::format("%s %d", v->type->name, ++v->type->numCreated);
		state->vehicles[vID] = v;

		OpenApoc::StateRef<OpenApoc::Vehicle> enemyVehicle = {state.get(), vID};
		OpenApoc::StateRef<OpenApoc::Vehicle> playerVehicle = {};

		std::list<OpenApoc::StateRef<OpenApoc::Agent>> agents;
		for (auto &a : state->agents)
		{

			if (a.second->type->role == OpenApoc::AgentType::Role::Soldier &&
			    a.second->owner == state->getPlayer())
			{
				agents.emplace_back(state.get(), a.second);
			}
		}

		OpenApoc::Battle::beginBattle(*state, false, org, agents, nullptr, playerVehicle,
		                              enemyVehicle);
		OpenApoc::Battle::enterBattle(*state);

		if (!test_gamestate_serialization(state))
		{
			LogError("Serialization test failed for in-battle game");
			return EXIT_FAILURE;
		}
		OpenApoc::Battle::finishBattle(*state);
		OpenApoc::Battle::exitBattle(*state);
	}

	LogInfo("test_serialize success");

	return EXIT_SUCCESS;
}
