#include "game/state/agent.h"
#include "game/state/gamestate.h"
#include <random>

namespace OpenApoc
{

const std::map<Agent::Type, UString> Agent::TypeMap = {
    {Type::Soldier, "soldier"},
    {Type::Physicist, "physicist"},
    {Type::BioChemist, "biochemist"},
    {Type::Engineer, "engineer"},
};

const std::map<Agent::Species, UString> Agent::SpeciesMap = {
    {Species::Human, "human"}, {Species::Mutant, "mutant"}, {Species::Android, "android"},
};

const std::map<Agent::Gender, UString> Agent::GenderMap = {
    {Gender::Male, "male"}, {Gender::Female, "female"},
};

template <> sp<Agent> StateObject<Agent>::get(const GameState &state, const UString &id)
{
	auto it = state.agents.find(id);
	if (it == state.agents.end())
	{
		LogError("No agent matching ID \"%s\"", id.cStr());
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<Agent>::getPrefix()
{
	static UString prefix = "AGENT_";
	return prefix;
}
template <> const UString &StateObject<Agent>::getTypeName()
{
	static UString name = "Agent";
	return name;
}
template <> const UString &StateObject<Agent>::getId(const GameState &state, const sp<Agent> ptr)
{
	static const UString emptyString = "";
	for (auto &a : state.agents)
	{
		if (a.second == ptr)
			return a.first;
	}
	LogError("No agent matching pointer %p", ptr.get());
	return emptyString;
}

template <typename T, typename Generator>
T probabilityMapRandomizer(Generator &g, const std::map<T, float> &probabilityMap)
{
	if (probabilityMap.empty())
	{
		LogError("Called with empty probabilityMap");
	}
	float total = 0.0f;
	for (auto &p : probabilityMap)
	{
		total += p.second;
	}
	std::uniform_real_distribution<float> dist(0, total);

	float val = dist(g);

	// Due to fp precision there's a small chance the total will be slightly more than the max,
	// so have a fallback just in case?
	T fallback = probabilityMap.begin()->first;
	total = 0.0f;

	for (auto &p : probabilityMap)
	{
		if (val < total + p.second)
		{
			return p.first;
		}
		total += p.second;
	}
	return fallback;
}

template <typename T, typename Generator> T listRandomiser(Generator &g, const std::list<T> &list)
{
	// we can't do index lookups in a list, so we just have to iterate N times
	if (list.size() == 1)
		return *list.begin();
	else if (list.empty())
	{
		LogError("Trying to randomize within empty list");
	}
	std::uniform_int_distribution<unsigned> dist(0, list.size() - 1);
	auto count = dist(g);
	auto it = list.begin();
	while (count)
	{
		it++;
		count--;
	}
	return *it;
}

template <typename T, typename Generator> T randBounds(Generator &g, T min, T max)
{
	if (min > max)
	{
		LogError("Bounds max < min");
	}
	// uniform_int_distribution is apparently undefined if min==max
	if (min == max)
		return min;
	std::uniform_int_distribution<T> dist(min, max);
	return dist(g);
}

StateRef<Agent> AgentGenerator::createAgent(GameState &state) const
{
	auto type = probabilityMapRandomizer(state.rng, this->type_chance);
	return this->createAgent(state, type);
}

StateRef<Agent> AgentGenerator::createAgent(GameState &state, Agent::Type type) const
{
	UString ID = UString::format("%s%u", Agent::getPrefix().cStr(), this->num_created);

	auto agent = mksp<Agent>();

	agent->type = type;
	agent->species = probabilityMapRandomizer(state.rng, this->species_chance);
	agent->gender = probabilityMapRandomizer(state.rng, this->gender_chance);

	auto firstNameList = this->first_names.find(agent->gender);
	if (firstNameList == this->first_names.end())
	{
		LogError("No first name list for gender");
		return nullptr;
	}

	auto firstName = listRandomiser(state.rng, firstNameList->second);
	auto secondName = listRandomiser(state.rng, this->second_names);
	agent->name = UString::format("%s %s", firstName, secondName);

	auto speciesPortraitMapIt = this->portraits.find(agent->species);
	if (speciesPortraitMapIt == this->portraits.end())
	{
		LogError("No portrait list for species");
		return nullptr;
	}

	auto &speciesPortraitMap = speciesPortraitMapIt->second;
	auto genderPortraitMapIt = speciesPortraitMap.find(agent->gender);
	if (genderPortraitMapIt == speciesPortraitMap.end())
	{
		LogError("No portrait list for gender");
		return nullptr;
	}

	agent->portrait = listRandomiser(state.rng, genderPortraitMapIt->second);

	auto minStatsIt = this->min_stats.find(agent->species);
	if (minStatsIt == this->min_stats.end())
	{
		LogError("No min_stats for species");
		return nullptr;
	}
	auto &minStats = minStatsIt->second;

	auto maxStatsIt = this->max_stats.find(agent->species);
	if (maxStatsIt == this->max_stats.end())
	{
		LogError("No max_stats for species");
		return nullptr;
	}
	auto &maxStats = maxStatsIt->second;

	AgentStats s;
	s.health = randBounds(state.rng, minStats.health, maxStats.health);
	s.accuracy = randBounds(state.rng, minStats.accuracy, maxStats.accuracy);
	s.reactions = randBounds(state.rng, minStats.reactions, maxStats.reactions);
	s.speed = randBounds(state.rng, minStats.speed, maxStats.speed);
	s.stamina = randBounds(state.rng, minStats.stamina, maxStats.stamina);
	s.bravery = randBounds(state.rng, minStats.bravery, maxStats.bravery);
	s.strength = randBounds(state.rng, minStats.strength, maxStats.strength);
	s.psi_energy = randBounds(state.rng, minStats.psi_energy, maxStats.psi_energy);
	s.psi_attack = randBounds(state.rng, minStats.psi_attack, maxStats.psi_attack);
	s.psi_defence = randBounds(state.rng, minStats.psi_defence, maxStats.psi_defence);
	s.physics_skill = randBounds(state.rng, minStats.physics_skill, maxStats.physics_skill);
	s.biochem_skill = randBounds(state.rng, minStats.biochem_skill, maxStats.biochem_skill);
	s.engineering_skill =
	    randBounds(state.rng, minStats.engineering_skill, maxStats.engineering_skill);

	agent->initial_stats = s;
	agent->current_stats = s;

	// Everything worked, add agent to stats
	this->num_created++;
	state.agents[ID] = agent;
	return {&state, ID};
}

} // namespace OpenApoc
