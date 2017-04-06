#pragma once

#include "framework/event.h"
#include "game/state/gameevent_types.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"

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

class GameEvent : public Event
{
  public:
	GameEventType type;

	GameEvent(GameEventType type);
	~GameEvent() override = default;
	virtual UString message() { return ""; }
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

	GameBaseEvent(GameEventType type, StateRef<Base> base);
	~GameBaseEvent() override = default;
	UString message() override;
};

class GameBuildingEvent : public GameEvent
{
  public:
	StateRef<Building> building;

	GameBuildingEvent(GameEventType type, StateRef<Building> building);
	~GameBuildingEvent() override = default;
};

class GameOrganisationEvent : public GameEvent
{
  public:
	StateRef<Organisation> organisation;

	GameOrganisationEvent(GameEventType type, StateRef<Organisation> organisation);
	~GameOrganisationEvent() override = default;
};

class GameAgentEvent : public GameEvent
{
  public:
	StateRef<Agent> agent;

	GameAgentEvent(GameEventType type, StateRef<Agent> agent);
	~GameAgentEvent() override = default;
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
}
