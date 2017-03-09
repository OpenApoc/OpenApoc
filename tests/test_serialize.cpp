#include "framework/configfile.h"
#include "framework/filesystem.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize.h"
#include <iostream>
#include <sstream>


// When I uncomment "using namespace" it says 
/* Error	C2678	binary '==': no operator found which takes a left - hand operand of type 'Concurrency::details::_Task_impl<_ReturnType>' 
(or there is no acceptable conversion)	test_serialize	E : \Projects\GitHub\OpenApoc\game\state\gamestate_serialize.h	106
*/


/*using namespace OpenApoc;

bool test_gamestate_serialization_roundtrip(sp<GameState> state, UString save_name)
{
	if (!state->saveGame(save_name))
	{
	
		LogWarning("Failed to save packed gamestate");
		return false;
	}

	auto read_gamestate = mksp<GameState>();
	if (!read_gamestate->loadGame(save_name))
	{
		LogWarning("Failed to load packed gamestate");
		return false;
	}

	if (*state != *read_gamestate)
	{
		LogWarning("Gamestate changed over serialization");
		
		//if (state->current_battle != read_gamestate->current_battle)
		{
			LogWarning("Battle changed over serialization");
			//if (state->current_battle->units != read_gamestate->current_battle->units)
			{
				LogWarning("Units changed over serialization");
			}		
			
			//if (state->current_battle->aiBlock != read_gamestate->current_battle->aiBlock)
			{
				LogWarning("AiBlock changed over serialization");
			}		
		}
		

		return false;
	}
	return true;
}

bool test_gamestate_serialization(sp<GameState> state)
{
	
	std::stringstream ss;
	ss << "openapoc_test_serialize-" << std::this_thread::get_id();
	auto tempPath = fs::temp_directory_path() / ss.str();
	UString pathString(tempPath.string());
	LogInfo("Writing temp state to \"%s\"", pathString);
	if (!test_gamestate_serialization_roundtrip(state, pathString))
	{
		LogWarning("Packed save test failed");
		return false;
	}

	fs::remove(tempPath);

	
	return true;
}
*/
int main(int argc, char **argv)
{
}
/*
	config().addPositionalArgument("common", "Common gamestate to load");
	config().addPositionalArgument("gamestate", "Gamestate to load");
	
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	auto gamestate_name = config().getString("gamestate");
	if (gamestate_name.empty())
	{
		std::cerr << "Must provide gamestate\n";
		config().showHelp();
		return EXIT_FAILURE;
	}
	auto common_name = config().getString("common");
	if (common_name.empty())
	{
		std::cerr << "Must provide common gamestate\n";
		config().showHelp();
		return EXIT_FAILURE;
	}

	Framework fw("OpenApoc", false);

	LogInfo("Loading \"%s\"", gamestate_name);

	auto state = mksp<GameState>();

	{
		auto state2 = mksp<GameState>();
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

	if (!test_gamestate_serialization(state))
	{
		LogError("Serialization test failed for started inited game");
		return EXIT_FAILURE;
	}

	LogInfo("Testing state with battle");
	{

		StateRef<Organisation> org = {state.get(), UString("ORG_ALIEN")};
		auto v = mksp<Vehicle>();
		auto vID = Vehicle::generateObjectID(*state);
		sp<VehicleType> vType;

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
		v->name = format("%s %d", v->type->name, ++v->type->numCreated);
		state->vehicles[vID] = v;

		StateRef<Vehicle> enemyVehicle = {state.get(), vID};
		StateRef<Vehicle> playerVehicle = {};

		std::list<StateRef<Agent>> agents;
		for (auto &a : state->agents)
		{
			if (a.second->type->role == AgentType::Role::Soldier &&
			    a.second->owner == state->getPlayer())
			{
				agents.emplace_back(state.get(), a.second);
			}
		}

		Battle::beginBattle(*state, org, agents, playerVehicle, enemyVehicle);
		Battle::enterBattle(*state);

		if (!test_gamestate_serialization(state))
		{
			LogError("Serialization test failed for in-battle game");
			return EXIT_FAILURE;
		}
		Battle::finishBattle(*state);
		Battle::exitBattle(*state);
	}

	LogInfo("test_serialize success");
	
	return EXIT_SUCCESS;
}
*/