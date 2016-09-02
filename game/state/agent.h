#pragma once
#include "framework/image.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include <map>

namespace OpenApoc
{

class Base;
class Organisation;

class AgentStats
{
  public:
	AgentStats() = default;
	int health = 0;
	int accuracy = 0;
	int reactions = 0;
	int speed = 0;
	int stamina = 0;
	int bravery = 0;
	int strength = 0;
	int psi_energy = 0;
	int psi_attack = 0;
	int psi_defence = 0;

	int physics_skill = 0;
	int biochem_skill = 0;
	int engineering_skill = 0;
};

class AgentPortrait
{
  public:
	sp<Image> photo;
	sp<Image> icon;
};

class Agent : public StateObject<Agent>
{
  public:
	Agent() = default;

	enum class Type
	{
		Soldier,
		Physicist,
		BioChemist,
		Engineer,
	};
	static const std::map<Type, UString> TypeMap;

	enum class Species
	{
		Human,
		Mutant,
		Android,
	};
	static const std::map<Species, UString> SpeciesMap;

	enum class Gender
	{
		Male,
		Female,
	};
	static const std::map<Gender, UString> GenderMap;

	UString name;
	AgentPortrait portrait;

	Type type = Type::Soldier;
	Species species = Species::Human;
	Gender gender = Gender::Male;

	AgentStats initial_stats;
	AgentStats current_stats;

	StateRef<Base> home_base;
	StateRef<Organisation> owner;

	bool assigned_to_lab = false;
};

class AgentGenerator
{
  public:
	AgentGenerator() = default;
	// Magic number to make unique agent IDs
	mutable unsigned int num_created = 0;
	// FIXME: I think there should be some kind of 'nationality' stuff going on here
	std::map<Agent::Gender, std::list<UString>> first_names;
	std::list<UString> second_names;

	std::map<Agent::Type, float> type_chance;
	std::map<Agent::Species, float> species_chance;
	std::map<Agent::Gender, float> gender_chance;

	std::map<Agent::Species, std::map<Agent::Gender, std::list<AgentPortrait>>> portraits;

	std::map<Agent::Species, AgentStats> min_stats;
	std::map<Agent::Species, AgentStats> max_stats;

	// Create an agent of random type
	StateRef<Agent> createAgent(GameState &state) const;
	// Create an agent of specified type
	StateRef<Agent> createAgent(GameState &state, Agent::Type type) const;
};

} // namespace OpenApoc
