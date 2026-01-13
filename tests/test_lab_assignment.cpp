#include "framework/configfile.h"
#include "framework/filesystem.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/facility.h"
#include "game/state/city/research.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize.h"
#include "game/state/shared/agent.h"
#include "library/strings.h"
#include <iostream>
#include <sstream>
#include <thread>

// Find a scientist agent owned by the player
static OpenApoc::StateRef<OpenApoc::Agent> findScientist(OpenApoc::sp<OpenApoc::GameState> state,
                                                         OpenApoc::AgentType::Role role)
{
	for (auto &agent : state->agents)
	{
		if (agent.second->owner == state->getPlayer() && agent.second->type->role == role &&
		    !agent.second->isAssignedToLab())
		{
			return {state.get(), agent.first};
		}
	}
	LogWarning("No scientist found");
	return {};
}

// Find a lab of the specified type in the player's bases
static OpenApoc::StateRef<OpenApoc::Lab> findLab(OpenApoc::sp<OpenApoc::GameState> state,
                                                 OpenApoc::ResearchTopic::Type type)
{
	for (auto &base : state->player_bases)
	{
		for (auto &facility : base.second->facilities)
		{
			if (facility->lab && facility->lab->type == type)
			{
				return {state.get(), facility->lab};
			}
		}
	}
	LogWarning("No lab found");
	return {};
}

// Create a second base for the player if one doesn't exist
// Returns the building of the second base
static OpenApoc::StateRef<OpenApoc::Building>
ensureSecondBase(OpenApoc::sp<OpenApoc::GameState> state)
{
	if (state->player_bases.size() >= 2)
	{
		// Return the building of any base that isn't the current one
		for (auto &base : state->player_bases)
		{
			if (base.second->building != state->current_base->building)
			{
				return base.second->building;
			}
		}
	}

	// Create a random new second base
	OpenApoc::sp<OpenApoc::Building> targetBuilding;
	for (auto &b : state->current_city->buildings)
	{
		// Building must have a base layout and not already have a base
		if (b.second->base_layout && !b.second->base && !b.second->initialInfiltration)
		{
			targetBuilding = b.second;
			break;
		}
	}

	if (!targetBuilding)
	{
		LogWarning("No eligible building found for second base");
		return {};
	}

	// Create the new base
	OpenApoc::StateRef<OpenApoc::Building> buildingRef{state.get(), targetBuilding};
	auto base = OpenApoc::mksp<OpenApoc::Base>(*state, buildingRef);
	// Init it with the starting layout to ensure we have the required facilities
	base->startingBase(*state);
	base->name = "Base " + OpenApoc::Strings::fromInteger(state->player_bases.size() + 1);

	OpenApoc::UString baseId = OpenApoc::Base::getPrefix() +
	                           OpenApoc::Strings::fromInteger(state->player_bases.size() + 1);
	state->player_bases[baseId] = base;

	targetBuilding->owner = state->getPlayer();
	targetBuilding->base = {state.get(), base};

	LogInfo("Created second base '{0}' in building '{1}'", base->name, targetBuilding->name);
	return buildingRef;
}

// Test basic lab assignment and unassignment
static bool test_lab_assignment_basic(OpenApoc::sp<OpenApoc::GameState> state)
{
	LogInfo("Testing basic lab assignment...");

	auto scientist = findScientist(state, OpenApoc::AgentType::Role::BioChemist);
	if (!scientist)
	{
		LogError("No unassigned BioChemist found, skipping basic assignment test");
		return true;
	}

	auto lab = findLab(state, OpenApoc::ResearchTopic::Type::BioChem);
	if (!lab)
	{
		LogWarning("No BioChem lab found, skipping basic assignment test");
		return true;
	}

	if (scientist->isAssignedToLab())
	{
		LogError("Scientist should not be assigned to lab initially");
		return false;
	}

	scientist->lab_assigned = lab;
	lab->assigned_agents.push_back(scientist);

	if (!scientist->isAssignedToLab())
	{
		LogError("Scientist should be assigned to lab after assignment");
		return false;
	}
	if (scientist->lab_assigned != lab)
	{
		LogError("Scientist's lab_assigned should point to the lab");
		return false;
	}

	bool found = false;
	for (const auto &a : lab->assigned_agents)
	{
		if (a == scientist)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		LogError("Scientist should be in lab's assigned_agents list");
		return false;
	}

	OpenApoc::Lab::removeAgent(lab, scientist);

	if (scientist->isAssignedToLab())
	{
		LogError("Scientist should not be assigned after removeAgent");
		return false;
	}
	if (scientist->lab_assigned != nullptr)
	{
		LogError("Scientist's lab_assigned should be nullptr after removeAgent");
		return false;
	}

	for (const auto &a : lab->assigned_agents)
	{
		if (a == scientist)
		{
			LogError("Scientist should not be in lab's assigned_agents list after removeAgent");
			return false;
		}
	}

	LogInfo("Basic lab assignment test passed");
	return true;
}

// Test that transfers agents and ensure's they're correctly removed from any assigned labs
static bool test_transfer_removes_from_lab(OpenApoc::sp<OpenApoc::GameState> state)
{
	LogInfo("Testing transfer removes from lab...");

	auto scientist = findScientist(state, OpenApoc::AgentType::Role::Physicist);
	if (!scientist)
	{
		LogError("No unassigned Physicist found, skipping transfer test");
		return true;
	}

	auto lab = findLab(state, OpenApoc::ResearchTopic::Type::Physics);
	if (!lab)
	{
		LogWarning("No Physics lab found, skipping transfer test");
		return true;
	}

	auto targetBuilding = ensureSecondBase(state);
	if (!targetBuilding)
	{
		LogError("Failed to create or find second base for transfer test");
		return false;
	}

	if (targetBuilding == scientist->homeBuilding)
	{
		LogError("Target building should be different from scientist's home building");
		return false;
	}

	scientist->lab_assigned = lab;
	lab->assigned_agents.push_back(scientist);

	if (!scientist->isAssignedToLab())
	{
		LogError("Scientist should be assigned to lab before transfer");
		return false;
	}

	size_t countBefore = lab->assigned_agents.size();

	scientist->transfer(*state, targetBuilding);

	if (scientist->isAssignedToLab())
	{
		LogError("Scientist should not be assigned to lab after transfer");
		return false;
	}
	if (scientist->lab_assigned != nullptr)
	{
		LogError("Scientist's lab_assigned should be nullptr after transfer");
		return false;
	}

	for (const auto &a : lab->assigned_agents)
	{
		if (a == scientist)
		{
			LogError("Scientist should not be in lab's assigned_agents list after transfer");
			return false;
		}
	}

	if (lab->assigned_agents.size() != countBefore - 1)
	{
		LogError("Lab's assigned_agents count should decrease by 1 after transfer");
		return false;
	}

	LogInfo("Transfer removes from lab test passed");
	return true;
}

// Test that death removes agent from any assgigned lab
static bool test_die_removes_from_lab(OpenApoc::sp<OpenApoc::GameState> state)
{
	LogInfo("Testing death removes from lab...");

	auto scientist = findScientist(state, OpenApoc::AgentType::Role::Engineer);
	if (!scientist)
	{
		LogError("No unassigned Engineer found, skipping death test");
		return true;
	}

	auto lab = findLab(state, OpenApoc::ResearchTopic::Type::Engineering);
	if (!lab)
	{
		LogWarning("No Engineering lab found, skipping death test");
		return true;
	}

	scientist->lab_assigned = lab;
	lab->assigned_agents.push_back(scientist);

	if (!scientist->isAssignedToLab())
	{
		LogError("Scientist should be assigned to lab before death");
		return false;
	}

	size_t countBefore = lab->assigned_agents.size();

	scientist->die(*state, true);

	if (scientist->lab_assigned != nullptr)
	{
		LogError("Dead scientist's lab_assigned should be nullptr");
		return false;
	}

	for (const auto &a : lab->assigned_agents)
	{
		if (a == scientist)
		{
			LogError("Dead scientist should not be in lab's assigned_agents list");
			return false;
		}
	}

	if (lab->assigned_agents.size() != countBefore - 1)
	{
		LogError("Lab's assigned_agents count should decrease by 1 after death");
		return false;
	}

	LogInfo("Death removes from lab test passed");
	return true;
}

// Test serialization roundtrip with lab assignment
static bool test_lab_assignment_serialization(OpenApoc::sp<OpenApoc::GameState> state)
{
	LogInfo("Testing lab assignment serialization...");

	auto scientist = findScientist(state, OpenApoc::AgentType::Role::BioChemist);
	if (!scientist)
	{
		LogError("No unassigned BioChemist found, skipping serialization test");
		return true;
	}

	auto lab = findLab(state, OpenApoc::ResearchTopic::Type::BioChem);
	if (!lab)
	{
		LogWarning("No BioChem lab found, skipping serialization test");
		return true;
	}

	scientist->lab_assigned = lab;
	lab->assigned_agents.push_back(scientist);

	OpenApoc::UString scientistId = scientist.id;
	OpenApoc::UString labId = lab.id;

	std::stringstream ss;
	ss << "openapoc_test_lab_assignment-" << std::this_thread::get_id();
	auto tempPath = fs::temp_directory_path() / ss.str();
	OpenApoc::UString pathString(tempPath.string());

	LogInfo("Saving state to \"{0}\"", pathString);
	if (!state->saveGame(pathString))
	{
		LogError("Failed to save gamestate");
		return false;
	}

	auto loadedState = OpenApoc::mksp<OpenApoc::GameState>();
	if (!loadedState->loadGame(pathString))
	{
		LogError("Failed to load gamestate");
		fs::remove(tempPath);
		return false;
	}

	fs::remove(tempPath);

	OpenApoc::StateRef<OpenApoc::Agent> loadedScientist{loadedState.get(), scientistId};
	OpenApoc::StateRef<OpenApoc::Lab> loadedLab{loadedState.get(), labId};

	if (!loadedScientist)
	{
		LogError("Scientist not found in loaded state");
		return false;
	}
	if (!loadedLab)
	{
		LogError("Lab not found in loaded state");
		return false;
	}

	if (!loadedScientist->isAssignedToLab())
	{
		LogError("Scientist should be assigned to lab after load");
		return false;
	}
	if (loadedScientist->lab_assigned != loadedLab)
	{
		LogError("Scientist's lab_assigned should point to the lab after load");
		return false;
	}

	bool found = false;
	for (const auto &a : loadedLab->assigned_agents)
	{
		if (a.id == scientistId)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		LogError("Scientist should be in lab's assigned_agents list after load");
		return false;
	}

	LogInfo("Lab assignment serialization test passed");
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

	LogInfo("Loading common gamestate \"{0}\"", common_name);

	auto state = OpenApoc::mksp<OpenApoc::GameState>();
	if (!state->loadGame(common_name))
	{
		LogError("Failed to load common gamestate");
		return EXIT_FAILURE;
	}

	LogInfo("Loading gamestate \"{0}\"", gamestate_name);
	if (!state->loadGame(gamestate_name))
	{
		LogError("Failed to load supplied gamestate");
		return EXIT_FAILURE;
	}

	// Start and init the game to have agents and labs available
	state->loadGame(common_name);
	state->loadGame(gamestate_name);
	state->startGame();
	state->initState();
	state->fillPlayerStartingProperty();

	if (!test_lab_assignment_basic(state))
	{
		LogError("Basic lab assignment test failed");
		return EXIT_FAILURE;
	}

	// We can re-use the state so long as we don't end up transferring/killing more scientists than
	// we start with

	if (!test_transfer_removes_from_lab(state))
	{
		LogError("Transfer removes from lab test failed");
		return EXIT_FAILURE;
	}

	if (!test_die_removes_from_lab(state))
	{
		LogError("Die removes from lab test failed");
		return EXIT_FAILURE;
	}

	if (!test_lab_assignment_serialization(state))
	{
		LogError("Lab assignment serialization test failed");
		return EXIT_FAILURE;
	}

	LogInfo("test_lab_assignment success - all tests passed");
	return EXIT_SUCCESS;
}
