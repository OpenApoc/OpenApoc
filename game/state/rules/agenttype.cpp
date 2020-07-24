#include "game/state/rules/agenttype.h"
#include "framework/framework.h"
#include "game/state/battle/ai/aitype.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/agentmission.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/facility.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/shared/aequipment.h"
#include "game/state/shared/organisation.h"
#include "game/state/tilemap/tileobject_scenery.h"
#include "library/strings_format.h"
#include <glm/glm.hpp>

namespace OpenApoc
{

template <> sp<AgentType> StateObject<AgentType>::get(const GameState &state, const UString &id)
{
	auto it = state.agent_types.find(id);
	if (it == state.agent_types.end())
	{
		LogError("No agent_type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<AgentType>::getPrefix()
{
	static UString prefix = "AGENTTYPE_";
	return prefix;
}
template <> const UString &StateObject<AgentType>::getTypeName()
{
	static UString name = "AgentType";
	return name;
}

template <>
const UString &StateObject<AgentType>::getId(const GameState &state, const sp<AgentType> ptr)
{
	static const UString emptyString = "";
	for (auto &a : state.agent_types)
	{
		if (a.second == ptr)
			return a.first;
	}
	LogError("No agent_type matching pointer %p", static_cast<void *>(ptr.get()));
	return emptyString;
}

template <>
sp<AgentBodyType> StateObject<AgentBodyType>::get(const GameState &state, const UString &id)
{
	auto it = state.agent_body_types.find(id);
	if (it == state.agent_body_types.end())
	{
		LogError("No agent_body_type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<AgentBodyType>::getPrefix()
{
	static UString prefix = "AGENTBODYTYPE_";
	return prefix;
}
template <> const UString &StateObject<AgentBodyType>::getTypeName()
{
	static UString name = "AgentBodyType";
	return name;
}

template <>
const UString &StateObject<AgentBodyType>::getId(const GameState &state,
                                                 const sp<AgentBodyType> ptr)
{
	static const UString emptyString = "";
	for (auto &a : state.agent_body_types)
	{
		if (a.second == ptr)
			return a.first;
	}
	LogError("No agent_type matching pointer %p", static_cast<void *>(ptr.get()));
	return emptyString;
}

template <>
sp<AgentEquipmentLayout> StateObject<AgentEquipmentLayout>::get(const GameState &state,
                                                                const UString &id)
{
	auto it = state.agent_equipment_layouts.find(id);
	if (it == state.agent_equipment_layouts.end())
	{
		LogError("No agent_body_type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<AgentEquipmentLayout>::getPrefix()
{
	static UString prefix = "AGENTEQUIPMENTLAYOUT_";
	return prefix;
}
template <> const UString &StateObject<AgentEquipmentLayout>::getTypeName()
{
	static UString name = "AgentEquipmentLayout";
	return name;
}

template <>
const UString &StateObject<AgentEquipmentLayout>::getId(const GameState &state,
                                                        const sp<AgentEquipmentLayout> ptr)
{
	static const UString emptyString = "";
	for (auto &a : state.agent_equipment_layouts)
	{
		if (a.second == ptr)
			return a.first;
	}
	LogError("No agent_type matching pointer %p", static_cast<void *>(ptr.get()));
	return emptyString;
}

AgentType::AgentType() : aiType(AIType::None) {}

EquipmentSlotType AgentType::getArmorSlotType(BodyPart bodyPart)
{
	switch (bodyPart)
	{
		case BodyPart::Body:
			return EquipmentSlotType::ArmorBody;
			break;
		case BodyPart::Legs:
			return EquipmentSlotType::ArmorLegs;
			break;
		case BodyPart::Helmet:
			return EquipmentSlotType::ArmorHelmet;
			break;
		case BodyPart::LeftArm:
			return EquipmentSlotType::ArmorLeftHand;
			break;
		case BodyPart::RightArm:
			return EquipmentSlotType::ArmorRightHand;
			break;
	}
	LogError("Unknown body part?");
	return EquipmentSlotType::General;
}

EquipmentLayoutSlot *AgentType::getFirstSlot(EquipmentSlotType type)
{
	for (auto &s : equipment_layout->slots)
	{
		if (s.type == type)
		{
			return &s;
		}
	}
	return nullptr;
}

BodyState AgentBodyType::getFirstAllowedState()
{
	auto bodyState = BodyState::Standing;
	if (allowed_body_states.find(bodyState) == allowed_body_states.end())
	{
		bodyState = BodyState::Flying;
	}
	if (allowed_body_states.find(bodyState) == allowed_body_states.end())
	{
		bodyState = BodyState::Kneeling;
	}
	if (allowed_body_states.find(bodyState) == allowed_body_states.end())
	{
		bodyState = BodyState::Prone;
	}
	if (allowed_body_states.find(bodyState) == allowed_body_states.end())
	{
		LogError("Body type cannot Stand, Fly, Kneel or go Prone! WTF!?");
	}
	return bodyState;
}

} // namespace OpenApoc
