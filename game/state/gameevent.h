#pragma once

#include "framework/event.h"
#include "game/state/gameeventtypes.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <map>

namespace OpenApoc
{

class Vehicle;
class Base;
class Building;
class Organisation;
class Agent;
class Battle;
class ResearchTopic;
class Lab;
class Facility;
class Organisation;

class GameEvent : public Event
{
  public:
	GameEventType type;

	static const std::map<GameEventType, UString> optionsMap;

	GameEvent(GameEventType type);
	~GameEvent() override = default;
	virtual UString message();
};

class GameVehicleEvent : public GameEvent
{
  public:
	StateRef<Vehicle> vehicle;
	StateRef<Vehicle> actor;

	GameVehicleEvent(GameEventType type, StateRef<Vehicle> vehicle,
	                 StateRef<Vehicle> actor = nullptr);
	~GameVehicleEvent() override = default;
	UString message() override;
};

class GameBaseEvent : public GameEvent
{
  public:
	StateRef<Base> base;
	StateRef<Organisation> actor;
	bool flag = false;

	GameBaseEvent(GameEventType type, StateRef<Base> base, StateRef<Organisation> actor = nullptr,
	              bool flag = false);
	~GameBaseEvent() override = default;
	UString message() override;
};

class GameBuildingEvent : public GameEvent
{
  public:
	StateRef<Building> building;
	StateRef<Organisation> actor;

	GameBuildingEvent(GameEventType type, StateRef<Building> building,
	                  StateRef<Organisation> actor = nullptr);
	~GameBuildingEvent() override = default;
	UString message() override;
};

class GameOrganisationEvent : public GameEvent
{
  public:
	StateRef<Organisation> organisation;

	GameOrganisationEvent(GameEventType type, StateRef<Organisation> organisation);
	~GameOrganisationEvent() override = default;
};

class GameDefenseEvent : public GameEvent
{
  public:
	StateRef<Base> base;
	StateRef<Organisation> organisation;

	GameDefenseEvent(GameEventType type, StateRef<Base> base, StateRef<Organisation> organisation);
	~GameDefenseEvent() override = default;
};

class GameAgentEvent : public GameEvent
{
  public:
	StateRef<Agent> agent;
	bool flag;

	GameAgentEvent(GameEventType type, StateRef<Agent> agent, bool flag = false);
	~GameAgentEvent() override = default;
	UString message() override;
};

class GameSomethingDiedEvent : public GameEvent
{
  public:
	UString messageInner;
	Vec3<int> location;

	GameSomethingDiedEvent(GameEventType type, UString name, UString actor, Vec3<int> location);
	GameSomethingDiedEvent(GameEventType type, UString name, Vec3<int> location);
	~GameSomethingDiedEvent() override = default;
	UString message() override;
};

class GameResearchEvent : public GameEvent
{
  public:
	StateRef<ResearchTopic> topic;
	StateRef<Lab> lab;

	GameResearchEvent(GameEventType type, StateRef<ResearchTopic> topic, StateRef<Lab> lab);
	~GameResearchEvent() override = default;
};

class GameManufactureEvent : public GameEvent
{
  public:
	StateRef<ResearchTopic> topic;
	StateRef<Lab> lab;
	unsigned done;
	unsigned goal;

	GameManufactureEvent(GameEventType type, StateRef<ResearchTopic> topic, unsigned done,
	                     unsigned goal, StateRef<Lab> lab);
	~GameManufactureEvent() override = default;
};

class GameFacilityEvent : public GameEvent
{
  public:
	sp<Base> base;
	sp<Facility> facility;

	GameFacilityEvent(GameEventType type, sp<Base> base, sp<Facility> facility);
	~GameFacilityEvent() override = default;
};

class GameBattleEvent : public GameEvent
{
  public:
	sp<Battle> battle;

	GameBattleEvent(GameEventType type, sp<Battle> battle);
	~GameBattleEvent() override = default;
	UString message() override;
};

class GameLocationEvent : public GameEvent
{
  public:
	Vec3<int> location;

	GameLocationEvent(GameEventType type, Vec3<int> location);
	~GameLocationEvent() override = default;
};
} // namespace OpenApoc
