#pragma once
#include "framework/event.h"
#include "game/state/gameevent_types.h"
#include "game/state/stateobject.h"

namespace OpenApoc
{

class Vehicle;
class Base;
class Building;
class Organisation;
class Agent;
class ResearchTopic;
class Lab;

class GameEvent : public Event
{
  public:
	GameEventType type;

	GameEvent(GameEventType type) : Event(EVENT_GAME_STATE), type(type) {}
	~GameEvent() override = default;
};

class GameVehicleEvent : public GameEvent
{
  public:
	StateRef<Vehicle> vehicle;

	GameVehicleEvent(GameEventType type, StateRef<Vehicle> vehicle)
	    : GameEvent(type), vehicle(vehicle)
	{
	}
	~GameVehicleEvent() override = default;
};

class GameBaseEvent : public GameEvent
{
  public:
	StateRef<Base> base;

	GameBaseEvent(GameEventType type, StateRef<Base> base) : GameEvent(type), base(base) {}
	~GameBaseEvent() override = default;
};

class GameBuildingEvent : public GameEvent
{
  public:
	StateRef<Building> building;

	GameBuildingEvent(GameEventType type, StateRef<Building> building)
	    : GameEvent(type), building(building)
	{
	}
	~GameBuildingEvent() override = default;
};

class GameOrganisationEvent : public GameEvent
{
  public:
	StateRef<Organisation> organisation;

	GameOrganisationEvent(GameEventType type, StateRef<Organisation> organisation)
	    : GameEvent(type), organisation(organisation)
	{
	}
	~GameOrganisationEvent() override = default;
};

class GameAgentEvent : public GameEvent
{
  public:
	StateRef<Agent> agent;

	GameAgentEvent(GameEventType type, StateRef<Agent> agent) : GameEvent(type), agent(agent) {}
	~GameAgentEvent() override = default;
};

class GameResearchEvent : public GameEvent
{
  public:
	StateRef<ResearchTopic> topic;
	StateRef<Lab> lab;

	GameResearchEvent(GameEventType type, StateRef<ResearchTopic> topic, StateRef<Lab> lab)
	    : GameEvent(type), topic(topic), lab(lab)
	{
	}
	~GameResearchEvent() override = default;
};
}
