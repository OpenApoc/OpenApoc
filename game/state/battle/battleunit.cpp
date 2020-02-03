#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/state/battle/battleunit.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/sound.h"
#include "game/state/battle/ai/unitaihelper.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleitem.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlecommonsamplelist.h"
#include "game/state/rules/battle/battleunitanimationpack.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/rules/city/facilitytype.h"
#include "game/state/shared/aequipment.h"
#include "game/state/shared/projectile.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include "game/state/tilemap/tileobject_shadow.h"
#include "library/line.h"
#include "library/strings_format.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace OpenApoc
{

namespace
{
static const std::set<TileObject::Type> mapPartSet = {
    TileObject::Type::Ground, TileObject::Type::LeftWall, TileObject::Type::RightWall,
    TileObject::Type::Feature};
static const std::set<TileObject::Type> unitSet = {TileObject::Type::Unit};
} // namespace

namespace
{
static const std::map<Vec2<int>, int> facing_dir_map = {{{0, -1}, 0}, {{1, -1}, 1}, {{1, 0}, 2},
                                                        {{1, 1}, 3},  {{0, 1}, 4},  {{-1, 1}, 5},
                                                        {{-1, 0}, 6}, {{-1, -1}, 7}};
static const std::map<int, Vec2<int>> dir_facing_map = {{0, {0, -1}}, {1, {1, -1}}, {2, {1, 0}},
                                                        {3, {1, 1}},  {4, {0, 1}},  {5, {-1, 1}},
                                                        {6, {-1, 0}}, {7, {-1, -1}}};
} // namespace

template <> sp<BattleUnit> StateObject<BattleUnit>::get(const GameState &state, const UString &id)
{
	auto it = state.current_battle->units.find(id);
	if (it == state.current_battle->units.end())
	{
		LogError("No agent_type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<BattleUnit>::getPrefix()
{
	static UString prefix = "BATTLEUNIT_";
	return prefix;
}
template <> const UString &StateObject<BattleUnit>::getTypeName()
{
	static UString name = "BattleUnit";
	return name;
}
template <>
const UString &StateObject<BattleUnit>::getId(const GameState &state, const sp<BattleUnit> ptr)
{
	static const UString emptyString = "";
	for (auto &a : state.current_battle->units)
	{
		if (a.second == ptr)
			return a.first;
	}
	LogError("No battleUnit matching pointer %p", ptr.get());
	return emptyString;
}

void BattleUnit::init(GameState &state)
{
	owner = agent->owner;
	agent->unit = {&state, id};
	aiList.init(state, *this);
}

void BattleUnit::destroy()
{
	targetUnit.clear();
	focusUnit.clear();
	focusedByUnits.clear();
	psiTarget.clear();
	brainSucker.clear();

	if (agent)
	{
		agent->unit.clear();
	}
	visibleUnits.clear();
	visibleEnemies.clear();
	aiList.aiList.clear();

	for (auto &m : missions)
	{
		m->targetUnit.clear();
	}
}

void BattleUnit::removeFromSquad(Battle &battle)
{
	if (squadNumber != -1)
	{
		battle.forces[owner].removeAt(squadNumber, squadPosition);
	}
}

bool BattleUnit::assignToSquad(Battle &battle, int squadNumber, int squadPosition)
{
	if (squadNumber == -1)
	{
		for (int i = 0; i < (int)battle.forces[owner].squads.size(); i++)
		{
			auto &squad = battle.forces[owner].squads[i];
			if (squad.getNumUnits() < 6)
			{
				if (squadPosition == -1)
				{
					return battle.forces[owner].insert(i, shared_from_this());
				}
				else
				{
					return battle.forces[owner].insertAt(i, squadPosition, shared_from_this());
				}
			}
		}
		return false;
	}
	else
	{
		if (squadPosition == -1)
		{
			return battle.forces[owner].insert(squadNumber, shared_from_this());
		}
		else
		{
			return battle.forces[owner].insertAt(squadNumber, squadPosition, shared_from_this());
		}
	}
}

bool BattleUnit::isFatallyWounded()
{
	for (auto &e : fatalWounds)
	{
		if (e.second > 0)
		{
			return true;
		}
	}
	return false;
}

void BattleUnit::setPosition(GameState &state, const Vec3<float> &pos, bool goal)
{
	auto oldPosition = position;
	position = pos;
	if (!tileObject)
	{
		LogError("setPosition called on unit with no tile object");
		return;
	}

	tileObject->setPosition(pos);
	if (shadowObject)
	{
		shadowObject->setPosition(tileObject->getCenter());
	}

	if (oldPosition != position)
	{
		// Notify battle that there's action going on at this position
		state.current_battle->notifyAction(position, {&state, id});
		if ((Vec3<int>)oldPosition != (Vec3<int>)position)
		{
			// Notify motion scanners
			state.current_battle->notifyScanners(position);
			tilesMoved++;
			// Spread hazards if this agent does so
			// (this overrides enzyme hazards)
			if (agent->type->spreadHazardDamageType)
			{
				state.current_battle->placeHazard(
				    state, owner, {&state, id}, agent->type->spreadHazardDamageType, oldPosition,
				    agent->type->spreadHazardDamageType->hazardType->getLifetime(state),
				    randBoundsInclusive(state.rng, agent->type->spreadHazardMinPower,
				                        agent->type->spreadHazardMaxPower),
				    agent->type->spreadHazardTTLDivizor, false);
			}
			// Spawn enzyme hazards
			else if (enzymeDebuffIntensity > 0)
			{
				spawnEnzymeSmoke(state);
			}
		}
	}

	if (goal)
	{
		onReachGoal(state);
	}
}

Vec3<float> BattleUnit::getVelocity() const
{
	Vec3<float> targetVelocity = velocity;
	// If moving instead use movement speed
	if (!atGoal && isMoving())
	{
		// Normalized Vector towards target position
		targetVelocity = goalPosition - position;
		if (targetVelocity.x != 0.0f || targetVelocity.y != 0.0f || targetVelocity.z != 0.0f)
		{
			targetVelocity = glm::normalize(targetVelocity);
			// Account for unit speed and movement state
			float speedMult = agent->modified_stats.getMovementSpeed();
			if (current_movement_state == MovementState::Running)
			{
				speedMult *= 2.0f;
			}
			else if (current_body_state == BodyState::Prone)
			{
				speedMult *= 0.666f;
			}
			targetVelocity *= speedMult;
			targetVelocity *= (float)TICK_SCALE / (float)TICKS_PER_UNIT_TRAVELLED_BATTLEUNIT;
		}
	}
	return targetVelocity;
}

void BattleUnit::refreshUnitVisibility(GameState &state)
{
	for (auto &entry : state.current_battle->units)
	{
		auto unit = entry.second;
		if (!unit->isConscious())
		{
			continue;
		}
		unit->refreshUnitVision(state, !unit->isWithinVision(position), {&state, id});
	}
}

bool BattleUnit::isWithinVision(Vec3<int> pos)
{
	auto diff = pos - (Vec3<int>)position;
	// Distance quick check
	if (diff.x * diff.x + diff.y * diff.y > VIEW_DISTANCE * VIEW_DISTANCE)
	{
		return false;
	}
	// Static units have 360 vision
	if (agent->type->bodyType->allowed_movement_states.size() == 1)
	{
		return true;
	}
	// Facing: Any, check if we're at the correct side
	if (diff.x * facing.x < 0 || diff.y * facing.y < 0)
	{
		return false;
	}
	// Facing: Diagonally
	if (facing.x != 0 && facing.y != 0)
	{
		// Nothing to be done, we already checked above
	}
	// Facing: Along one of axes
	else
	{
		// Facing: Along X
		if (facing.x != 0)
		{
			// Already checked if we're at the correct side above
			// Now we only need to check if we're inside the cone
			if (std::abs(diff.x) < std::abs(diff.y))
			{
				return false;
			}
		}
		// Facing: Along Y
		else
		{
			// Already checked if we're at the correct side above
			// Now we only need to check if we're inside the cone
			if (std::abs(diff.x) > std::abs(diff.y))
			{
				return false;
			}
		}
	}
	return true;
}

void BattleUnit::calculateVisionToTerrain(GameState &state)
{
	auto &battle = *state.current_battle;

	static const int lazyLimit = 5 * 9;
	std::set<int> discoveredBlocks;
	auto &visibleBlocks = battle.visibleBlocks.at(owner);

	// Update unit's vision of los block he's standing in
	{
		auto idx = battle.getLosBlockID(position.x, position.y, position.z);
		if (!visibleBlocks.at(idx))
		{
			visibleBlocks.at(idx) = true;
			discoveredBlocks.insert(idx);
		}
	}

	auto blocksToCheck = std::set<int>();
	int totalChecks = 0;
	// Calc los to other blocks we haven't seen yet
	for (int idx = 0; idx < (int)visibleBlocks.size(); idx++)
	{
		// Block already seen
		if (visibleBlocks.at(idx))
		{
			continue;
		}

		totalChecks++;
		blocksToCheck.insert(idx);
		if (totalChecks >= lazyLimit)
		{
			break;
		}
	}

	if (totalChecks >= lazyLimit)
	{
		calculateVisionToLosBlocksLazy(state, discoveredBlocks);
	}
	else
	{
		calculateVisionToLosBlocks(state, discoveredBlocks, blocksToCheck);
	}

	// Reveal all discovered blocks
	for (auto &idx : discoveredBlocks)
	{
		auto l = battle.losBlocks.at(idx);
		for (int x = l->start.x; x < l->end.x; x++)
		{
			for (int y = l->start.y; y < l->end.y; y++)
			{
				for (int z = l->start.z; z < l->end.z; z++)
				{
					battle.setVisible(owner, x, y, z);
				}
			}
		}
	}
}

void BattleUnit::calculateVisionToLosBlocks(GameState &state, std::set<int> &discoveredBlocks,
                                            std::set<int> &blocksToCheck)
{
	auto eyesPos = getEyeLocation();
	auto &battle = *state.current_battle;
	auto &map = *battle.map;

	auto &visibleBlocks = battle.visibleBlocks.at(owner);
	for (auto &idx : blocksToCheck)
	{
		// Get block and its center
		auto &l = *battle.losBlocks.at(idx);
		auto centerXY = Vec3<int>{(l.start.x + l.end.x) / 2, (l.start.y + l.end.y) / 2, 0};
		// Set target to center
		bool targetFound = false;
		auto target = centerXY;
		// Set target's Z to our level (or closest possible)
		int posZ = position.z;
		if (posZ >= l.start.z && posZ < l.end.z)
		{
			target.z = posZ;
		}
		else if (posZ < l.start.z)
		{
			target.z = l.start.z;
		}
		else
		{
			target.z = l.end.z - 1;
		}
		// Try to target center of LOS block (on our Z level)
		if (isWithinVision(target))
		{
			targetFound = true;
		}
		// Try to target point within that is closest to our sight's middlepoint
		else
		{
			// Get point in the middle of our sight forward
			int dist =
			    facing.x != 0 && facing.y != 0 ? VIEW_DISTANCE * 100 / 141 / 2 : VIEW_DISTANCE / 2;
			auto sightMiddleXY = (Vec3<int>)position;
			sightMiddleXY.x += facing.x * dist;
			sightMiddleXY.y += facing.y * dist;
			// Get point closest to that point
			target.x =
			    std::abs(l.start.x - sightMiddleXY.x) < std::abs(l.end.x - 1 - sightMiddleXY.x)
			        ? l.start.x
			        : l.end.x - 1;
			target.y =
			    std::abs(l.start.y - sightMiddleXY.y) < std::abs(l.end.y - 1 - sightMiddleXY.y)
			        ? l.start.y
			        : l.end.y - 1;
			if (posZ >= l.start.z && posZ < l.end.z)
			{
				target.z = posZ;
			}
			else if (posZ < l.start.z)
			{
				target.z = l.start.z;
			}
			else
			{
				target.z = l.end.z - 1;
			}
			// Try to target that point
			if (isWithinVision(target))
			{
				targetFound = true;
			}
		}

		// If target is found then we can try to los to this block
		if (targetFound)
		{
			auto c = map.findCollision(eyesPos, {target.x + 0.5f, target.y + 0.5f, target.z + 0.5f},
			                           mapPartSet, tileObject, true, false, VIEW_DISTANCE);

			// FIXME: Handle collisions with left/right/ground that prevent seeing inside
			// If going positive on axes, we must shorten our beam a little bit, so that if
			// collision was with a wall or ground, it would not consider a block as seen

			if (!c.outOfRange && (!c || l.contains(c.position)))
			{
				visibleBlocks.at(idx) = true;
				discoveredBlocks.insert(idx);
			}
		}
	}
}

void BattleUnit::calculateVisionToLosBlocksLazy(GameState &state, std::set<int> &discoveredBlocks)
{
	auto eyesPos = getEyeLocation();
	auto &battle = *state.current_battle;
	auto &map = *battle.map;

	auto &visibleBlocks = battle.visibleBlocks.at(owner);
	auto &tileToLosBlock = battle.tileToLosBlock;

	// Basically, we're checking in five directions on Z-scale, and in five directions on  XY scale
	// Values are picked so that atan of these values give relatively even angle distributions

	// First value is multiplier for XY part of vector, second value is vector's Z
	static const std::vector<Vec2<float>> zTarget = {{0.18f, 1.0f},  {1.0f, 1.0f},  {1.0f, 0.55f},
	                                                 {1.0f, 0.18f},  {1.0f, 0.0f},  {1.0f, -0.18f},
	                                                 {1.0f, -0.55f}, {1.0f, -1.0f}, {0.18f, -1.0f}};
	// First value is X, second value is Y
	static const std::vector<Vec2<float>> diagTarget = {
	    {0.18f, 1.0f}, {0.55f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.55f}, {1.0f, 0.18f}};

	// Value for the coordinate which is zero in the facing (Y if facing along X etc.)
	static const std::vector<float> dirTarget = {-0.82f, -0.45f, 0.0f, 0.45f, 0.82f};

	for (int z = 0; z < 9; z++)
	{
		for (int xy = 0; xy < 5; xy++)
		{
			Vec3<float> targetVector;
			if (facing.x != 0 && facing.y != 0)
			{
				targetVector = {(float)facing.x * diagTarget.at(xy).x * zTarget.at(z).x,
				                (float)facing.y * diagTarget.at(xy).y * zTarget.at(z).x,
				                zTarget.at(z).y};
			}
			else if (facing.x != 0)
			{
				targetVector = {(float)facing.x * zTarget.at(z).x,
				                dirTarget.at(xy) * zTarget.at(z).x, zTarget.at(z).y};
			}
			else
			{
				targetVector = {dirTarget.at(xy) * zTarget.at(z).x,
				                (float)facing.y * zTarget.at(z).x, zTarget.at(z).y};
			}
			targetVector = glm::normalize(targetVector) * (float)VIEW_DISTANCE;

			auto c = map.findCollision(eyesPos, eyesPos + targetVector, mapPartSet, tileObject,
			                           true, false, VIEW_DISTANCE, true);

			for (auto &t : c.passedTiles)
			{
				auto idx = tileToLosBlock.at(t.z * battle.size.x * battle.size.y +
				                             t.y * battle.size.x + t.x);
				if (!visibleBlocks.at(idx))
				{
					visibleBlocks.at(idx) = true;
					discoveredBlocks.insert(idx);
				}
			}
		}
	}
}

bool BattleUnit::calculateVisionToUnit(GameState &state, BattleUnit &u)
{
	auto eyesPos = getEyeLocation();
	auto &map = *state.current_battle->map;

	// Unit unconscious, we own this unit or can't see it, skip
	if (!u.isConscious() || u.owner == owner || !isWithinVision(u.position))
	{
		return false;
	}
	auto target = u.tileObject->getCenter();
	if (u.isLarge())
	{
		// Offset search for large units as they can get caught up in ground
		// that is supposed to allow units to go through it but blocks LOS
		auto targetvVectorDelta = glm::normalize(target - eyesPos) * 0.75f;
		target -= targetvVectorDelta;
	}
	auto c = map.findCollision(eyesPos, target, mapPartSet, tileObject, true, false,
	                           VIEW_DISTANCE / (u.isCloaked() ? 2 : 1));
	if (c || c.outOfRange)
	{
		return false;
	}
	return true;
}

void BattleUnit::calculateVisionToUnits(GameState &state)
{
	for (auto &entry : state.current_battle->units)
	{
		if (calculateVisionToUnit(state, *entry.second))
		{
			visibleUnits.emplace(&state, entry.first);
		}
	}
}

void BattleUnit::refreshUnitVision(GameState &state, bool forceBlind,
                                   StateRef<BattleUnit> targetUnit)
{
	auto &battle = *state.current_battle;
	auto lastVisibleUnits = visibleUnits;
	auto ticks = state.gameTime.getTicks();
	visibleUnits.clear();
	visibleEnemies.clear();

	// Vision is actually updated only if conscious, otherwise we clear visible units and that's it
	if (isConscious())
	{
		if (!targetUnit)
		{
			if (!forceBlind)
			{
				calculateVisionToTerrain(state);
				calculateVisionToUnits(state);
			}
		}
		else
		{
			visibleUnits = lastVisibleUnits;
			if (!forceBlind && calculateVisionToUnit(state, *targetUnit))
			{
				if (visibleUnits.find(targetUnit) == visibleUnits.end())
				{
					visibleUnits.insert(targetUnit);
				}
			}
			else
			{
				if (visibleUnits.find(targetUnit) != visibleUnits.end())
				{
					visibleUnits.erase(targetUnit);
				}
			}
		}
	}

	// Add newly visible units to owner's list and enemy list
	for (auto &vu : visibleUnits)
	{
		// owner's visible units list
		if (lastVisibleUnits.find(vu) == lastVisibleUnits.end() &&
		    battle.visibleUnits[owner].find(vu) == battle.visibleUnits[owner].end())
		{
			battle.visibleUnits[owner].insert(vu);
			if (owner == state.current_battle->currentPlayer &&
			    owner->isRelatedTo(vu->owner) == Organisation::Relation::Hostile &&
			    (battle.lastVisibleTime[owner].find(vu) == battle.lastVisibleTime[owner].end() ||
			     battle.lastVisibleTime[owner][vu] + TICKS_SUPPRESS_SPOTTED_MESSAGES <= ticks))
			{
				vu->sendAgentEvent(state, GameEventType::HostileSpotted);
				state.current_battle->notifyAction(vu->position, vu);
			}
		}
		// battle and units's visible enemies list
		if (owner->isRelatedTo(vu->owner) == Organisation::Relation::Hostile)
		{
			visibleEnemies.insert(vu);
			battle.visibleEnemies[owner].insert(vu);
		}
	}

	// See if someone else sees a unit we stopped seeing
	for (auto &lvu : lastVisibleUnits)
	{
		if (visibleUnits.find(lvu) == visibleUnits.end())
		{
			bool someoneElseSees = false;
			for (auto &u : state.current_battle->units)
			{
				if (u.second->owner != owner)
				{
					continue;
				}
				if (u.second->visibleUnits.find(lvu) != u.second->visibleUnits.end())
				{
					someoneElseSees = true;
					break;
				}
			}
			if (!someoneElseSees)
			{
				battle.visibleUnits[owner].erase(lvu);
				battle.visibleEnemies[owner].erase(lvu);
				battle.lastVisibleTime[owner][lvu] = ticks;
			}
		}
	}
}

void BattleUnit::refreshUnitVisibilityAndVision(GameState &state)
{
	refreshUnitVision(state);
	refreshUnitVisibility(state);
}

void BattleUnit::resetGoal()
{
	goalPosition = position;
	goalFacing = facing;
	atGoal = true;
}

void BattleUnit::onReachGoal(GameState &state)
{
	// Remember who seen us before (for interrupt)
	std::set<StateRef<BattleUnit>> enemiesThatSeenUsBefore;
	if (state.current_battle->mode == Battle::Mode::TurnBased)
	{
		auto srthis = StateRef<BattleUnit>(&state, id);
		for (auto &u : state.current_battle->units)
		{
			if (u.second->visibleEnemies.find(srthis) != u.second->visibleEnemies.end())
			{
				enemiesThatSeenUsBefore.insert(srthis);
			}
		}
	}
	refreshUnitVisibilityAndVision(state);
	// Interrupt
	if (state.current_battle->mode == Battle::Mode::TurnBased)
	{
		auto srthis = StateRef<BattleUnit>(&state, id);
		for (auto &u : state.current_battle->units)
		{
			if (u.second->visibleEnemies.find(srthis) != u.second->visibleEnemies.end())
			{
				// Mutual surprise rule: unit can interrupt us only if either is true:
				// - he has seen us before
				// - we don't see him now
				if (enemiesThatSeenUsBefore.find({&state, u.first}) !=
				        enemiesThatSeenUsBefore.end() ||
				    visibleEnemies.find({&state, u.first}) == visibleEnemies.end())
				{
					state.current_battle->giveInterruptChanceToUnit(
					    state, {&state, id}, {&state, u.first}, agent->getReactionValue());
				}
			}
		}
	}
}

int BattleUnit::getAttackCost(GameState &state, AEquipment &item, Vec3<int> tile)
{
	std::ignore = state;
	int totalCost = 0;

	// Step 1: Turning cost
	auto targetFacing = BattleUnitMission::getFacing(*this, tile);
	bool turning = goalFacing != targetFacing;
	if (turning)
	{
		int curFacing = facing_dir_map.at(goalFacing);
		int tarFacing = facing_dir_map.at(targetFacing);
		if (tarFacing > 7)
			tarFacing -= 8;

		int clockwiseDistance = tarFacing - curFacing;
		if (clockwiseDistance < 0)
		{
			clockwiseDistance += 8;
		}
		int counterClockwiseDistance = curFacing - tarFacing;
		if (counterClockwiseDistance < 0)
		{
			counterClockwiseDistance += 8;
		}
		totalCost = std::min(clockwiseDistance, counterClockwiseDistance) * getTurnCost();
	}

	// Step 2: Body state change cost
	if (turning)
	{
		if (target_body_state == BodyState::Prone)
		{
			totalCost += getBodyStateChangeCost(BodyState::Prone, BodyState::Kneeling);
		}
		if (movement_mode == MovementMode::Prone && kneeling_mode == KneelingMode::None)
		{
			totalCost += getBodyStateChangeCost(BodyState::Kneeling, BodyState::Prone);
		}
	}

	// Step 3: Actual cost to fire the weapon
	totalCost += item.getFireCost(fire_aiming_mode, initialTU);

	return totalCost;
}

void BattleUnit::setFocus(GameState &state, StateRef<BattleUnit> unit)
{
	StateRef<BattleUnit> sru = {&state, id};
	if (focusUnit)
	{
		auto it =
		    std::find(focusUnit->focusedByUnits.begin(), focusUnit->focusedByUnits.end(), sru);
		if (it != focusUnit->focusedByUnits.end())
		{
			focusUnit->focusedByUnits.erase(it);
		}
		else
		{
			LogError("Inconsistent focusUnit/focusBy!");
		}
	}
	focusUnit = unit;
	focusUnit->focusedByUnits.push_back(sru);
}

bool BattleUnit::startAttacking(GameState &state, WeaponStatus status)
{
	switch (state.current_battle->mode)
	{
		case Battle::Mode::TurnBased:
		{
			if (moraleState == MoraleState::Normal)
			{
				// In Turn based we cannot fire both hands (unless zerking)
				if (status == WeaponStatus::FiringBothHands)
				{
					// Right hand has priority
					auto rhItem = agent->getFirstItemInSlot(EquipmentSlotType::RightHand);
					if (rhItem && rhItem->canFire(state))
					{
						status = WeaponStatus::FiringRightHand;
					}
					else
					{
						// We don't care what's in the left hand,
						// we will just cancel firing in update() if there's nothing to fire
						status = WeaponStatus::FiringLeftHand;
					}
				}
				// Check TU
				auto weapon = (status == WeaponStatus::FiringRightHand)
				                  ? agent->getFirstItemInSlot(EquipmentSlotType::RightHand)
				                  : agent->getFirstItemInSlot(EquipmentSlotType::LeftHand);
				if (!weapon || !weapon->canFire(state, targetTile) ||
				    !canAfford(state, getAttackCost(state, *weapon, targetTile), true, true))
				{
					return false;
				}
			}
			break;
		}
		case Battle::Mode::RealTime:
		{
			// Start firing both hands if added one hand to another
			if ((weaponStatus == WeaponStatus::FiringLeftHand &&
			     status == WeaponStatus::FiringRightHand) ||
			    (weaponStatus == WeaponStatus::FiringRightHand &&
			     status == WeaponStatus::FiringLeftHand))
			{
				status = WeaponStatus::FiringBothHands;
			}
			break;
		}
	}

	weaponStatus = status;
	ticksUntillNextTargetCheck = 0;
	timesTargetMIA = 0;
	return true;
}

bool BattleUnit::startAttacking(GameState &state, StateRef<BattleUnit> unit, WeaponStatus status)
{
	// Attack on a unit that is downed is replaced with an attack on the occupied tile's ground
	if (!unit->isConscious())
	{
		return startAttacking(state, unit->tileObject->getVoxelCentrePosition(), status, true);
	}
	// Attack on a friendly unit is replaced with an attack on the occupied tile's center
	if (unit->owner == owner)
	{
		return startAttacking(state, unit->tileObject->getVoxelCentrePosition(), status);
	}
	targetTile = unit->position;
	if (!startAttacking(state, status))
	{
		return false;
	}
	targetUnit = unit;
	targetingMode = TargetingMode::Unit;
	return true;
}

bool BattleUnit::startAttacking(GameState &state, Vec3<int> tile, WeaponStatus status,
                                bool atGround)
{
	targetTile = tile;
	if (!startAttacking(state, status))
	{
		return false;
	}
	targetingMode = atGround ? TargetingMode::TileGround : TargetingMode::TileCenter;
	return true;
}

void BattleUnit::stopAttacking()
{
	weaponStatus = WeaponStatus::NotFiring;
	targetingMode = TargetingMode::NoTarget;
	targetUnit.clear();
	ticksUntillNextTargetCheck = 0;
}

WeaponStatus BattleUnit::canAttackUnit(GameState &state, sp<BattleUnit> unit)
{
	return canAttackUnit(state, unit, agent->getFirstItemInSlot(EquipmentSlotType::RightHand),
	                     agent->getFirstItemInSlot(EquipmentSlotType::LeftHand));
}

WeaponStatus BattleUnit::canAttackUnit(GameState &state, sp<BattleUnit> unit,
                                       sp<AEquipment> rightHand, sp<AEquipment> leftHand)
{
	bool realTime = state.current_battle->mode == Battle::Mode::RealTime;
	auto targetPosition = unit->tileObject->getVoxelCentrePosition();
	if (hasLineToUnit(unit))
	{
		// One of held weapons is in range
		bool rightCanFire =
		    rightHand && rightHand->canFire(state, targetPosition) &&
		    (realTime ||
		     canAfford(state, getAttackCost(state, *rightHand, unit->position), true, true));
		bool leftCanFire =
		    leftHand && leftHand->canFire(state, targetPosition) &&
		    (realTime ||
		     canAfford(state, getAttackCost(state, *leftHand, unit->position), true, true));
		if (rightCanFire && leftCanFire)
		{
			return WeaponStatus::FiringBothHands;
		}
		else if (rightCanFire)
		{
			return WeaponStatus::FiringRightHand;
		}
		else if (leftCanFire)
		{
			return WeaponStatus::FiringLeftHand;
		}
	}
	return WeaponStatus::NotFiring;
}

bool BattleUnit::hasLineToUnit(const sp<BattleUnit> unit, bool useLOS) const
{
	auto muzzleLocation = getMuzzleLocation();
	auto targetPosition = unit->tileObject->getVoxelCentrePosition();
	if (unit->isLarge())
	{
		// Offset search for large units as they can get caught up in ground
		// that is supposed to allow units to go through it but blocks LOS
		auto targetvVectorDelta = glm::normalize(targetPosition - muzzleLocation) * 0.75f;
		targetPosition -= targetvVectorDelta;
	}
	return hasLineToPosition(targetPosition, useLOS);
}

bool BattleUnit::hasLineToPosition(Vec3<float> targetPosition, bool useLOS) const
{
	auto muzzleLocation = getMuzzleLocation();
	// Map part that prevents Line to target
	auto cMap = tileObject->map.findCollision(muzzleLocation, targetPosition, mapPartSet,
	                                          tileObject, useLOS);
	// Unit that prevents Line to target
	auto cUnitObj =
	    useLOS ? Collision()
	           : tileObject->map.findCollision(muzzleLocation, targetPosition, unitSet, tileObject);
	auto cUnit = cUnitObj ? std::static_pointer_cast<TileObjectBattleUnit>(cUnitObj.obj)->getUnit()
	                      : nullptr;
	// Condition:
	// No map part blocks Line
	return !cMap
	       // No unit blocks Line
	       && (!cUnit ||
	           owner->isRelatedTo(cUnit->owner) == Organisation::Relation::Hostile
	           // If our head blocks brainsucker on it - no problem, hit will go versus brainsucker
	           // anyway
	           || cUnit->brainSucker);
}

int BattleUnit::getPsiCost(PsiStatus status, bool attack)
{
	switch (status)
	{
		case PsiStatus::NotEngaged:
			LogError("Invalid value NotEngaged for psiStatus in getPsiCost()");
			return 0;
		case PsiStatus::Control:
			return attack ? 32 : 4;
		case PsiStatus::Panic:
			return attack ? 10 : 3;
		case PsiStatus::Stun:
			return attack ? 16 : 5;
		case PsiStatus::Probe:
			return attack ? 8 : 3;
	}
	LogError("Unexpected Psi Status in getPsiCost()");
	return 0;
}

int BattleUnit::getPsiChance(StateRef<BattleUnit> target, PsiStatus status,
                             StateRef<AEquipmentType> item)
{
	if (status == PsiStatus::NotEngaged)
	{
		LogError("Invalid value NotEngaged for psiStatus in getPsiChance()");
		return 0;
	}
	auto e1 = agent->getFirstItemInSlot(EquipmentSlotType::RightHand);
	auto e2 = agent->getFirstItemInSlot(EquipmentSlotType::LeftHand);
	if (e1 && e1->type != item)
	{
		e1 = nullptr;
	}
	if (e2 && e2->type != item)
	{
		e2 = nullptr;
	}
	auto bender = e1 ? e1 : e2;
	auto cost = getPsiCost(status);
	if (!bender || agent->modified_stats.psi_energy < cost || !hasLineToUnit(target, true))
	{
		return 0;
	}

	// Psi chance as per Wong's Guide, confirmed by Mell
	/*
	                     100*attack*(100-defense)
	success rate = --------------------------------------
	                 attack*(100-defense) + 100*defense

	           psiattack rating * 40
	attack = ---------------------------
	          initiation cost of action

	Note: As tested by Mell, Probe is min 20%
	*/
	int attack = agent->modified_stats.psi_attack * 40 / cost;
	int defense = target->agent->modified_stats.psi_defence;
	int chance = 0;
	if (attack != 0 || defense != 0)
	{
		chance = (100 * attack * (100 - defense)) / (attack * (100 - defense) + 100 * defense);
	}
	if (status == PsiStatus::Probe && attack != 0)
	{
		chance = std::max(20, chance);
	}
	return chance;
}

bool BattleUnit::startAttackPsi(GameState &state, StateRef<BattleUnit> target, PsiStatus status,
                                StateRef<AEquipmentType> item)
{
	bool realTime = state.current_battle->mode == Battle::Mode::RealTime;
	if (agent->modified_stats.psi_energy < getPsiCost(status))
	{
		return false;
	}
	bool success = startAttackPsiInternal(state, target, status, item);
	agent->modified_stats.psi_energy -= getPsiCost(status);
	if (success)
	{
		fw().soundBackend->playSample(pickRandom(state.rng, *psiSuccessSounds), position);
		if (!realTime)
		{
			psiTarget->applyPsiAttack(state, *this, psiStatus, psiItem, false);
			if (psiStatus != PsiStatus::Control)
			{
				stopAttackPsi(state);
			}
		}
		return true;
	}
	else
	{
		fw().soundBackend->playSample(pickRandom(state.rng, *psiFailSounds), position);
		return false;
	}
}

bool BattleUnit::startAttackPsiInternal(GameState &state, StateRef<BattleUnit> target,
                                        PsiStatus status, StateRef<AEquipmentType> item)
{
	if (agent->getAnimationPack()->useFiringAnimationForPsi)
	{
		setHandState(HandState::Firing);
	}
	int chance = getPsiChance(target, status, item);
	int roll = randBoundsExclusive(state.rng, 0, 100);
	experiencePoints.psi_attack++;
	experiencePoints.psi_energy++;
	LogWarning("Psi Attack #%d Roll %d Chance %d %s Attacker %s Target %s", (int)status, roll,
	           chance, roll < chance ? (UString) "SUCCESS" : (UString) "FAILURE", id, target->id);
	if (roll >= chance)
	{
		return false;
	}
	experiencePoints.psi_attack += 2;
	experiencePoints.psi_energy += 2;

	// Attack hit, apply effects

	if (psiTarget)
	{
		stopAttackPsi(state);
	}
	psiTarget = target;
	psiStatus = status;
	psiItem = item;
	target->psiAttackers[id] = status;
	ticksAccumulatedToNextPsiCheck = 0;
	target->applyPsiAttack(state, *this, status, item, true);

	return true;
}

void BattleUnit::applyPsiAttack(GameState &state, BattleUnit &attacker, PsiStatus status,
                                StateRef<AEquipmentType> item, bool impact)
{
	std::ignore = item;
	bool realTime = state.current_battle->mode == Battle::Mode::RealTime;
	if (impact)
	{
		sendAgentEvent(state,
		               status == PsiStatus::Control ? GameEventType::AgentPsiControlled
		                                            : GameEventType::AgentPsiAttacked,
		               true);
	}
	bool finished = false;
	do
	{
		// In turn based, you attack over an over until you're done, and each attack costs extra
		if (!realTime && !impact && status != PsiStatus::Probe)
		{
			int cost = getPsiCost(status, true);
			if (attacker.agent->modified_stats.psi_energy < cost)
			{
				finished = true;
				continue;
			}
			attacker.agent->modified_stats.psi_energy -= cost;
		}
		// Actually apply attack
		switch (status)
		{
			case PsiStatus::Panic:
				if (!impact)
				{
					agent->modified_stats.loseMorale(realTime ? 8 : 4);
					if (agent->modified_stats.morale == 0)
					{
						std::set<UString> panickers;
						for (auto attacker : psiAttackers)
						{
							if (attacker.second == PsiStatus::Panic)
							{
								panickers.emplace(attacker.first);
							}
						}
						for (auto s : panickers)
						{
							StateRef<BattleUnit>(&state, s)->stopAttackPsi(state);
						}
					}
				}
			case PsiStatus::Stun:
				if (!impact)
				{
					applyDamageDirect(state, realTime ? 7 : 4, false, BodyPart::Body, 9001);
				}
				break;
			case PsiStatus::Control:
				if (impact)
				{
					changeOwner(state, attacker.owner);
					agent->modified_stats.loseMorale(100);
				}
				break;
			case PsiStatus::Probe:
				if (impact)
				{
					if (attacker.owner == state.current_battle->currentPlayer &&
					    attacker.agent->type->allowsDirectControl)
					{
						sendAgentEvent(state, GameEventType::AgentPsiProbed);
					}
				}
				break;
			case PsiStatus::NotEngaged:
				LogError("Invalid value NotEngaged for psiStatus in applyPsiAttack()");
				return;
		}
	} while (!finished && !realTime && !impact && status != PsiStatus::Probe);
}

void BattleUnit::stopAttackPsi(GameState &state)
{
	switch (psiStatus)
	{
		case PsiStatus::Control:
			if (psiTarget->owner == psiTarget->agent->owner)
			{
				break;
			}
			psiTarget->changeOwner(state, psiTarget->agent->owner);
			psiTarget->sendAgentEvent(state, GameEventType::AgentPsiOver, true);
			break;
		case PsiStatus::Probe:
		case PsiStatus::Panic:
		case PsiStatus::Stun:
			// Nothing immediate to be done
			break;
		case PsiStatus::NotEngaged:
			return;
	}
	psiTarget->psiAttackers.erase(id);
	psiStatus = PsiStatus::NotEngaged;
	psiTarget.clear();
	psiItem.clear();
	ticksAccumulatedToNextPsiCheck = 0;
}

void BattleUnit::changeOwner(GameState &state, StateRef<Organisation> newOwner)
{
	if (owner == newOwner)
	{
		return;
	}
	refreshUnitVision(state, true);
	stopAttacking();
	stopAttackPsi(state);
	cancelMissions(state);
	removeFromSquad(*state.current_battle);
	owner = newOwner;
	assignToSquad(*state.current_battle);
	refreshUnitVisibilityAndVision(state);
}

void BattleUnit::setReserveShotMode(GameState &state, ReserveShotMode mode)
{
	reserve_shot_mode = mode;
	refreshReserveCost(state);
}

void BattleUnit::setReserveKneelMode(KneelingMode mode)
{
	if (!agent->isBodyStateAllowed(BodyState::Kneeling))
	{
		mode = KneelingMode::None;
	}
	reserve_kneel_mode = mode;
}

void BattleUnit::refreshReserveCost(GameState &state)
{
	reserveShotCost = 0;
	if (reserve_shot_mode == ReserveShotMode::None)
	{
		return;
	}
	auto e1 = agent->getFirstItemInSlot(EquipmentSlotType::RightHand);
	auto e2 = agent->getFirstItemInSlot(EquipmentSlotType::LeftHand);
	auto weapon = e1 && e1->canFire(state) ? e1 : (e2 && e2->canFire(state) ? e2 : nullptr);
	if (weapon)
	{
		reserveShotCost = weapon->getFireCost((WeaponAimingMode)reserve_shot_mode, initialTU);
	}
}

bool BattleUnit::canAfford(GameState &state, int cost, bool ignoreKneelReserve,
                           bool ignoreShootReserve) const
{
	if (state.current_battle->mode == Battle::Mode::RealTime || !isConscious())
	{
		return true;
	}
	// When not our turn we ignore reserve setting anyway
	ignoreKneelReserve =
	    ignoreKneelReserve || state.current_battle->currentActiveOrganisation != owner;
	ignoreShootReserve =
	    ignoreShootReserve || state.current_battle->currentActiveOrganisation != owner;
	if (!ignoreKneelReserve && reserve_kneel_mode != KneelingMode::None)
	{
		cost += getBodyStateChangeCost(BodyState::Standing, BodyState::Kneeling);
	}
	if (!ignoreShootReserve && reserve_shot_mode != ReserveShotMode::None)
	{
		cost += reserveShotCost;
	}
	return agent->modified_stats.time_units >= cost;
}

bool BattleUnit::spendTU(GameState &state, int cost, bool ignoreKneelReserve,
                         bool ignoreShootReserve, bool allowInterrupt)
{
	if (state.current_battle->mode == Battle::Mode::RealTime || !isConscious() || cost == 0)
	{
		return true;
	}
	if (!canAfford(state, cost, ignoreKneelReserve, ignoreShootReserve))
	{
		return false;
	}
	agent->modified_stats.time_units -= cost;
	state.current_battle->notifyAction(position, {&state, id});
	// If doing any action other than moving that triggers reaction, we give interrupt chance here
	// If moving we do not give it here, and instead give interrupt chance when changing tiles
	if (allowInterrupt)
	{
		state.current_battle->giveInterruptChanceToUnits(state, {&state, id},
		                                                 agent->getReactionValue());
	}
	return true;
}

void BattleUnit::spendRemainingTU(GameState &state, bool allowInterrupt)
{
	agent->modified_stats.time_units = 0;
	if (allowInterrupt)
	{
		state.current_battle->giveInterruptChanceToUnits(state, {&state, id},
		                                                 agent->getReactionValue());
	}
}

int BattleUnit::getPickupCost() const { return initialTU * 10 / 100; }

int BattleUnit::getThrowCost() const { return initialTU * 18 / 100; }

int BattleUnit::getMedikitCost() const { return initialTU * 375 / 1000; }

int BattleUnit::getMotionScannerCost() const { return initialTU * 10 / 100; }

int BattleUnit::getTeleporterCost() const { return initialTU * 55 / 100; }

int BattleUnit::getTurnCost() const { return 1; }

int BattleUnit::getBodyStateChangeCost(BodyState from, BodyState to) const
{
	// If not within these conditions, it costs nothing!
	switch (to)
	{
		case BodyState::Flying:
		case BodyState::Standing:
			switch (from)
			{
				case BodyState::Kneeling:
					return 8;
				case BodyState::Prone:
					return 16;
				default:
					return 0;
			}
			break;
		case BodyState::Kneeling:
			switch (from)
			{
				case BodyState::Prone:
				case BodyState::Standing:
				case BodyState::Flying:
					return 8;
				default:
					return 0;
			}
			break;
		case BodyState::Prone:
			switch (from)
			{
				case BodyState::Kneeling:
				case BodyState::Standing:
				case BodyState::Flying:
					return 8;
				default:
					return 0;
			}
			break;
		case BodyState::Throwing:
			return getThrowCost();
		default:
			return 0;
	}
}

void BattleUnit::beginTurn(GameState &state)
{
	std::ignore = state;
	tilesMoved = 0;
	agent->modified_stats.restoreTU();
	initialTU = agent->modified_stats.time_units;
	if (!missions.empty() && missions.front()->type == BattleUnitMission::Type::AcquireTU)
	{
		missions.front()->allowContinue = true;
	}
}

bool BattleUnit::isDead() const { return agent->isDead() || destroyed; }

bool BattleUnit::isUnconscious() const { return !isDead() && stunDamage >= agent->getHealth(); }

bool BattleUnit::isConscious() const
{
	return !isDead() && !retreated && stunDamage < agent->getHealth() &&
	       (current_body_state != BodyState::Downed || target_body_state != BodyState::Downed) &&
	       target_body_state != BodyState::Dead;
}

bool BattleUnit::isStatic() const
{
	return !falling && current_movement_state == MovementState::None;
}

bool BattleUnit::isBusy() const
{
	return !missions.empty() || isAttacking() ||
	       (getAIType() != AIType::Civilian && getAIType() != AIType::None &&
	        !visibleEnemies.empty());
}

bool BattleUnit::isAttacking() const { return weaponStatus != WeaponStatus::NotFiring; }

bool BattleUnit::isThrowing() const { return isDoing(BattleUnitMission::Type::ThrowItem); }

bool BattleUnit::isMoving() const
{
	return isDoing(BattleUnitMission::Type::GotoLocation) ||
	       isDoing(BattleUnitMission::Type::ReachGoal);
}

bool BattleUnit::isDoing(BattleUnitMission::Type missionType) const
{
	bool found = false;
	for (auto &m : missions)
	{
		if (m->type == missionType)
		{
			found = true;
			break;
		}
	}
	return found;
}

BattleUnitType BattleUnit::getType() const
{
	if (isLarge())
	{
		if (canFly())
		{
			return BattleUnitType::LargeFlyer;
		}
		else
		{
			return BattleUnitType::LargeWalker;
		}
	}
	else
	{
		if (canFly())
		{
			return BattleUnitType::SmallFlyer;
		}
		else
		{
			return BattleUnitType::SmallWalker;
		}
	}
}

bool BattleUnit::isAIControlled(GameState &state) const
{
	return owner != state.current_battle->currentPlayer;
}

bool BattleUnit::isCloaked() const { return cloakTicksAccumulated >= CLOAK_TICKS_REQUIRED_UNIT; }

AIType BattleUnit::getAIType() const
{
	switch (moraleState)
	{
		case MoraleState::Normal:
			return agent->type->aiType;
		case MoraleState::PanicFreeze:
			return AIType::PanicFreeze;
		case MoraleState::PanicRun:
			return AIType::PanicRun;
		case MoraleState::Berserk:
			return AIType::Berserk;
	}
	LogError("Unhandled morale state in getAIType()");
	return AIType::None;
}

bool BattleUnit::canFly() const
{
	return isConscious() && agent->isBodyStateAllowed(BodyState::Flying);
}

bool BattleUnit::canMove() const
{
	if (!isConscious() || agent->overEncumbred)
	{
		return false;
	}
	if (agent->isMovementStateAllowed(MovementState::Normal) ||
	    agent->isMovementStateAllowed(MovementState::Running))
	{
		return true;
	}
	return false;
}

bool BattleUnit::canProne(Vec3<int> pos, Vec2<int> fac) const
{
	if (isLarge())
	{
		LogError("Large unit attempting to go prone? WTF? Should large units ever acces this?");
		return false;
	}
	// Check if agent can go prone and stand in its current tile
	if (!agent->isBodyStateAllowed(BodyState::Prone) || !tileObject->getOwningTile()->getCanStand())
		return false;
	// Check if agent can put legs in the tile behind. Conditions
	// 1) Target tile provides standing ability
	// 2) Target tile height is not too big compared to current tile
	// 3) Target tile is passable
	// 4) Target tile has no unit occupying it (other than us)
	Vec3<int> legsPos = pos - Vec3<int>{fac.x, fac.y, 0};
	if ((legsPos.x >= 0) && (legsPos.x < tileObject->map.size.x) && (legsPos.y >= 0) &&
	    (legsPos.y < tileObject->map.size.y) && (legsPos.z >= 0) &&
	    (legsPos.z < tileObject->map.size.z))
	{
		auto bodyTile = tileObject->map.getTile(pos);
		auto legsTile = tileObject->map.getTile(legsPos);
		if (legsTile->canStand && bodyTile->canStand &&
		    std::abs(legsTile->height - bodyTile->height) <= 0.25f &&
		    legsTile->getPassable(false, agent->type->bodyType->height.at(BodyState::Prone)) &&
		    (legsPos == (Vec3<int>)position || !legsTile->getUnitIfPresent(true, true)))
		{
			return true;
		}
	}
	return false;
}

bool BattleUnit::canKneel() const
{
	if (!agent->isBodyStateAllowed(BodyState::Kneeling) ||
	    !tileObject->getOwningTile()->getCanStand(isLarge()))
		return false;
	return true;
}

void BattleUnit::addFatalWound(BodyPart fatalWoundPart) { fatalWounds[fatalWoundPart]++; }

void BattleUnit::applyDamageDirect(GameState &state, int damage, bool generateFatalWounds,
                                   BodyPart fatalWoundPart, int stunPower,
                                   StateRef<BattleUnit> attacker, bool violent)
{
	// Just a blank value for checks (if equal to this means no event)
	static auto NO_EVENT = GameEventType::AgentArrived;

	if (isDead())
	{
		return;
	}
	if (generateFatalWounds && agent->type->immuneToFatalWounds)
	{
		generateFatalWounds = false;
	}

	bool wasConscious = isConscious();
	bool fatal = false;

	auto eventType = NO_EVENT;
	// Deal stun damage
	if (stunPower > 0)
	{
		stunDamage += clamp(damage, 0, std::max(0, stunPower + agent->getMaxHealth() - stunDamage));
	}
	// Deal health damage
	else
	{
		bool lessThanOneThird = agent->modified_stats.health * 3 / agent->current_stats.health == 0;
		agent->modified_stats.health -= damage;
		agent->modified_stats.loseMorale(damage * 50 * (15 - agent->modified_stats.bravery / 10) /
		                                 agent->current_stats.health / 100);
		eventType = (agent->modified_stats.health * 3 / agent->current_stats.health == 0) &&
		                    !lessThanOneThird
		                ? GameEventType::AgentBadlyInjured
		                : GameEventType::AgentInjured;
	}

	// Generate fatal wounds
	if (generateFatalWounds && damage > agent->current_stats.health / 8 &&
	    randBoundsExclusive(state.rng, 0, 100) < 100 * damage / agent->current_stats.health)
	{
		addFatalWound(fatalWoundPart);
		eventType = GameEventType::AgentCriticallyWounded;
		fatal = true;
	}

	// Die or go unconscious
	if (isDead())
	{
		eventType = NO_EVENT; // cancel event on death
		die(state, attacker, violent);
		return;
	}
	else if (!isConscious() && wasConscious)
	{
		eventType = NO_EVENT; // cancel event on falling down
		fallUnconscious(state);
	}

	if (eventType != NO_EVENT)
	{
		sendAgentEvent(state, eventType, true);
	}

	if (wasConscious)
	{
		// Emit sound fatal wound
		if (fatal)
		{
			if (agent->type->fatalWoundSfx.find(agent->gender) !=
			        agent->type->fatalWoundSfx.end() &&
			    !agent->type->fatalWoundSfx.at(agent->gender).empty())
			{
				fw().soundBackend->playSample(
				    pickRandom(state.rng, agent->type->fatalWoundSfx.at(agent->gender)), position);
			}
		}
		// Emit sound wound
		else if (stunPower == 0)
		{
			if (agent->type->damageSfx.find(agent->gender) != agent->type->damageSfx.end() &&
			    !agent->type->damageSfx.at(agent->gender).empty())
			{
				fw().soundBackend->playSample(
				    pickRandom(state.rng, agent->type->damageSfx.at(agent->gender)), position);
			}
		}
	}

	return;
}

bool BattleUnit::applyDamage(GameState &state, int power, StateRef<DamageType> damageType,
                             BodyPart bodyPart, DamageSource source, StateRef<BattleUnit> attacker)
{
	if (damageType->doesImpactDamage())
	{
		fw().soundBackend->playSample(pickRandom(state.rng, *genericHitSounds), position);
	}

	// Calculate damage
	int damage = 0;
	if (damageType->effectType == DamageType::EffectType::Smoke) // smoke deals 1-3 stun damage
	{
		power = 2;
		damage = randDamage050150(state.rng, power);
	}
	else if (damageType->effectType == DamageType::EffectType::Fire)
	{
		switch (source)
		{
			case DamageSource::Impact:
			{
				static const std::list<int> damageDistribution = {0, 5, 6, 7, 8, 9, 10};
				damage = pickRandom(state.rng, damageDistribution);
				break;
			}
			case DamageSource::Hazard:
				damage = randBoundsInclusive(state.rng, 1, 12);
				break;
			case DamageSource::Debuff:
				damage = randBoundsInclusive(state.rng, 5, 10);
				break;
		}
	}
	else if (damageType->explosive) // explosive deals 50-150% damage
	{
		damage = randDamage050150(state.rng, power);
	}
	else if (config().getBool("OpenApoc.NewFeature.UFODamageModel"))
	{
		damage = randDamage000200(state.rng, power);
	}
	else
	{
		damage = randDamage050150(state.rng, power);
	}

	// Hit shield if present
	if (!damageType->ignore_shield)
	{
		auto shield = agent->getFirstShield(state);
		if (shield)
		{
			damage = damageType->dealDamage(damage, shield->type->damage_modifier);
			shield->ammo -= damage;
			// Shield destroyed
			if (shield->ammo <= 0)
			{
				agent->removeEquipment(state, shield);
			}
			state.current_battle->placeDoodad(shield->type->shield_graphic,
			                                  tileObject->getCenter());
			return true;
		}
	}

	// Apply enzyme if penetrated shields
	if (damageType->effectType == DamageType::EffectType::Enzyme)
	{
		enzymeDebuffIntensity += power * 2;
		enzymeDebuffTicksAccumulated = 0;
		applyEnzymeEffect(state);
	}

	// Find out armor type
	auto armor = agent->getArmor(bodyPart);
	int armorValue = 0;
	StateRef<DamageModifier> damageModifier;
	if (armor)
	{
		armorValue = armor->armor;
		damageModifier = armor->type->damage_modifier;
	}
	else
	{
		armorValue = agent->type->armor.at(bodyPart);
		damageModifier = agent->type->damage_modifier;
	}

	// Catch on fire
	if (damageType->effectType == DamageType::EffectType::Fire)
	{
		bool catchOnFire = false;
		switch (source)
		{
			case DamageSource::Impact:
				catchOnFire = damage > 0;
				break;
			case DamageSource::Hazard:
				catchOnFire = randBoundsExclusive(state.rng, 0, 100) <
				              damageType->dealDamage(40, damageModifier);
				break;
			case DamageSource::Debuff:
				break;
		}
		if (catchOnFire)
		{
			fireDebuffTicksRemaining =
			    (unsigned int)damageType->dealDamage(5 * TICKS_PER_TURN, damageModifier);
			fireDebuffTicksAccumulated = 0;
		}
	}
	// Smoke ignores armor value but does not ignore damage modifier
	damage = damageType->dealDamage(damage, damageModifier) -
	         (damageType->ignoresArmorValue() ? 0 : armorValue);

	if (this->owner == state.getPlayer())
	{
		damage = (double)damage * config().getFloat("OpenApoc.Cheat.DamageReceivedMultiplier");
	}
	if (attacker && attacker->owner == state.getPlayer())
	{
		damage = (double)damage * config().getFloat("OpenApoc.Cheat.DamageInflictedMultiplier");
	}

	// No damage
	if (damage <= 0)
	{
		return false;
	}

	// Smoke, fire and stun damage does not damage armor
	if (damageType->dealsArmorDamage() && armor)
	{
		// Armor damage
		int armorDamage = damage / 10 + 1;
		armor->armor -= armorDamage;
		// Armor destroyed
		if (armor->armor <= 0)
		{
			agent->removeEquipment(state, armor);
		}
	}

	// Apply damage according to type
	if (damageType->explosive && damageType->effectType == DamageType::EffectType::None)
	{
		// Deal 1/8 of explosive damage as stun
		int stunDamage = damage / 8;
		applyDamageDirect(state, stunDamage, false, bodyPart, power, attacker);
		damage -= stunDamage;
	}
	applyDamageDirect(state, damage, damageType->dealsFatalWounds(), bodyPart,
	                  damageType->dealsStunDamage() ? power : 0, attacker,
	                  !damageType->non_violent);

	return false;
}

BodyPart BattleUnit::determineBodyPartHit(StateRef<DamageType> damageType, Vec3<float> cposition,
                                          Vec3<float> direction)
{
	BodyPart bodyPartHit = BodyPart::Body;

	// FIXME: Ensure body part determination is correct
	// Assume top 25% is head, lower 25% is legs, and middle 50% is body/left/right
	float altitude = (cposition.z - position.z) * 40.0f / (float)getCurrentHeight();
	if (damageType->alwaysImpactsHead()) // gas deals damage to the head
	{
		bodyPartHit = BodyPart::Helmet;
	}
	else if (altitude > 0.75f)
	{
		bodyPartHit = BodyPart::Helmet;
	}
	else if (altitude < 0.25f)
	{
		bodyPartHit = BodyPart::Legs;
	}
	else
	{
		auto unitDir = glm::normalize(Vec3<float>{facing.x, facing.y, 0.0f});
		auto projectileDir = glm::normalize(Vec3<float>{direction.x, direction.y, 0.0f});
		auto cross = glm::cross(unitDir, projectileDir);
		int angle =
		    (int)((cross.z >= 0 ? -1 : 1) * glm::angle(unitDir, -projectileDir) / M_PI * 180.0f);
		if (angle > 45 && angle < 135)
		{
			bodyPartHit = BodyPart::RightArm;
		}
		else if (angle < -45 && angle > -135)
		{
			bodyPartHit = BodyPart::LeftArm;
		}
	}
	return bodyPartHit;
}

bool BattleUnit::handleCollision(GameState &state, Collision &c)
{
	std::ignore = state;

	// Corpses do not handle collision
	if (isDead())
		return false;

	if (!this->tileObject)
	{
		LogError("It's possible multiple projectiles hit the same tile in the same tick (?)");
		return false;
	}

	auto projectile = c.projectile.get();
	if (projectile)
	{
		auto partHit =
		    determineBodyPartHit(projectile->damageType, c.position, projectile->velocity);
		// If hit in helmet with brainsucker attached -> hit brainsucker instead
		if (brainSucker && partHit == BodyPart::Helmet)
		{
			brainSucker->handleCollision(state, c);
			return false;
		}
		else
		{
			state.current_battle->giveInterruptChanceToUnit(
			    state, {&state, id}, {&state, id},
			    projectile->firerUnit->agent->getReactionValue());
			notifyHit(position - glm::normalize(projectile->velocity) * 1.41f);
			if (projectile->firerUnit)
			{
				projectile->firerUnit->experiencePoints.accuracy++;
			}
			return applyDamage(state, projectile->damage, projectile->damageType, partHit,
			                   DamageSource::Impact, projectile->firerUnit);
		}
	}
	return false;
}

void BattleUnit::update(GameState &state, unsigned int ticks)
{
	bool realTime = state.current_battle->mode == Battle::Mode::RealTime;

	// Animate
	body_animation_ticks_static += ticks;

	// Destroyed or retreated units do not exist in the battlescape
	if (destroyed || retreated)
	{
		return;
	}

	// Update Items
	bool updatedShield = false;
	for (auto &item : agent->equipment)
	{
		if (item->type->type == AEquipmentType::Type::DisruptorShield &&
		    item->ammo < item->getPayloadType()->max_ammo)
		{
			if (updatedShield)
			{
				continue;
			}
			updatedShield = true;
		}
		item->update(state, ticks);
	}

	// Update Missions
	if (!this->missions.empty())
		this->missions.front()->update(state, *this, ticks);

	// [Update the Unit itself]

	if (realTime)
	{
		// Miscellaneous state updates, as well as unit's stats
		updateStateAndStats(state, ticks);
		// Unit regeneration
		updateRegen(state, ticks);
	}
	// Unit cloak - even in TB unit returns to cloaked after firing based on time
	updateCloak(state, ticks);
	// Unit events - was under fire, was requested to give way etc.
	updateEvents(state);
	// Idling: Auto-movement, auto-body change when idling
	updateIdling(state);
	// Crying: Enemies emit sounds periodically
	updateCrying(state);
	// Main bulk - update movement, body, hands and turning
	{
		bool wasUsingLift = usingLift;
		usingLift = false;

		// If not running we will consume these twice as fast, if prone thrice as
		unsigned int moveTicksRemaining =
		    ticks * agent->modified_stats.getMovementSpeed() * BASE_MOVETICKS_CONSUMPTION_RATE;
		unsigned int bodyTicksRemaining = ticks;
		unsigned int handsTicksRemaining = ticks;
		unsigned int turnTicksRemaining = ticks;

		// Unconscious units cannot move their hands or turn, they can only animate body or fall
		if (!isConscious())
		{
			handsTicksRemaining = 0;
			turnTicksRemaining = 0;
		}

		unsigned int lastMoveTicksRemaining = 0;
		unsigned int lastBodyTicksRemaining = 0;
		unsigned int lastHandsTicksRemaining = 0;
		unsigned int lastTurnTicksRemaining = 0;

		while (lastMoveTicksRemaining != moveTicksRemaining ||
		       lastBodyTicksRemaining != bodyTicksRemaining ||
		       lastHandsTicksRemaining != handsTicksRemaining ||
		       lastTurnTicksRemaining != turnTicksRemaining)
		{
			lastMoveTicksRemaining = moveTicksRemaining;
			lastBodyTicksRemaining = bodyTicksRemaining;
			lastHandsTicksRemaining = handsTicksRemaining;
			lastTurnTicksRemaining = turnTicksRemaining;

			updateCheckBeginFalling(state);
			updateBody(state, bodyTicksRemaining);
			updateHands(state, handsTicksRemaining);
			updateMovement(state, moveTicksRemaining, wasUsingLift);
			updateTurning(state, turnTicksRemaining, handsTicksRemaining);
			updateDisplayedItem(state);
		}
	}
	if (retreated)
	{
		return;
	}
	// Unit's attacking state
	updateAttacking(state, ticks);
	// Unit's psi attack state
	updatePsi(state, ticks);
	// AI
	updateAI(state, ticks);
	// Who else? :)
	triggerBrainsuckers(state);
}

void BattleUnit::updateTB(GameState &state)
{
	// Destroyed or retreated units do not exist in the battlescape
	if (destroyed || retreated)
	{
		return;
	}

	// Update Items
	bool updatedShield = false;
	for (auto &item : agent->equipment)
	{
		if (item->type->type == AEquipmentType::Type::DisruptorShield &&
		    item->ammo < item->getPayloadType()->max_ammo)
		{
			if (updatedShield)
			{
				continue;
			}
			updatedShield = true;
		}
		item->updateTB(state);
	}

	// Miscellaneous state updates, as well as unit's stats
	updateCloak(state, TICKS_PER_TURN);
	updateStateAndStats(state, TICKS_PER_TURN);
	updateRegen(state, TICKS_REGEN_PER_TURN);
}

void BattleUnit::updateCloak(GameState &state [[maybe_unused]], unsigned int ticks)
{
	if (isConscious())
	{
		auto e1 = agent->getFirstItemInSlot(EquipmentSlotType::LeftHand);
		auto e2 = agent->getFirstItemInSlot(EquipmentSlotType::RightHand);

		if (cloakTicksAccumulated < CLOAK_TICKS_REQUIRED_UNIT)
		{
			cloakTicksAccumulated += ticks;
		}
		if ((!e1 || e1->type->type != AEquipmentType::Type::CloakingField) &&
		    (!e2 || e2->type->type != AEquipmentType::Type::CloakingField))
		{
			cloakTicksAccumulated = 0;
		}
	}
}

void BattleUnit::updateStateAndStats(GameState &state, unsigned int ticks)
{
	bool realTime = state.current_battle->mode == Battle::Mode::RealTime;

	if (isDead())
	{
		return;
	}

	// Morale (units under mind control cannot have low morale event)
	if ((realTime || owner == state.current_battle->currentActiveOrganisation) && isConscious() &&
	    owner == agent->owner)
	{
		updateMorale(state, ticks);
	}

	// Fatal wounds / healing
	if (isFatallyWounded() && !isDead())
	{
		updateWoundsAndHealing(state, ticks);
	}

	// Process enzyme
	if (enzymeDebuffIntensity > 0)
	{
		enzymeDebuffTicksAccumulated += ticks;
		while (enzymeDebuffTicksAccumulated >= TICKS_PER_ENZYME_EFFECT && enzymeDebuffIntensity > 0)
		{
			enzymeDebuffTicksAccumulated -= TICKS_PER_ENZYME_EFFECT;

			applyEnzymeEffect(state);
		}
	}

	// Process fire
	if (fireDebuffTicksRemaining > 0)
	{
		fireDebuffTicksAccumulated += ticks;
		while (fireDebuffTicksAccumulated >= TICKS_PER_FIRE_EFFECT && fireDebuffTicksRemaining > 0)
		{
			fireDebuffTicksAccumulated -= TICKS_PER_FIRE_EFFECT;

			// Damage (power is irrelevant here)
			applyDamage(state, 1, {&state, "DAMAGETYPE_INCENDIARY"}, BodyPart::Body,
			            DamageSource::Debuff);

			// Finally, reduce debuff
			fireDebuffTicksRemaining -= TICKS_PER_FIRE_EFFECT;
		}
	}
}

void BattleUnit::updateMorale(GameState &state, unsigned int ticks)
{
	auto e1 = agent->getFirstItemInSlot(EquipmentSlotType::LeftHand);
	auto e2 = agent->getFirstItemInSlot(EquipmentSlotType::RightHand);

	if (moraleStateTicksRemaining > 0)
	{
		if (moraleStateTicksRemaining > ticks)
		{
			moraleStateTicksRemaining -= ticks;
		}
		else
		{
			// It seems in Apoc unit keeps panicking until reaches positive morale
			if (agent->modified_stats.morale >= 50)
			{
				moraleStateTicksRemaining = 0;
				moraleState = MoraleState::Normal;
				sendAgentEvent(state, GameEventType::AgentPanicOver, true);
				stopAttacking();
				cancelMissions(state, true);
				aiList.reset(state, *this);
			}
			else
			{
				moraleStateTicksRemaining += TICKS_PER_LOWMORALE_STATE;
				moraleStateTicksRemaining -= ticks;
				agent->modified_stats.morale += 15;
			}
		}
	}
	else
	{
		moraleTicksAccumulated += ticks;
		while (moraleTicksAccumulated >= LOWMORALE_CHECK_INTERVAL)
		{
			moraleTicksAccumulated -= LOWMORALE_CHECK_INTERVAL;

			if (randBoundsExclusive(state.rng, 0, 100) >= 100 - 2 * agent->modified_stats.morale)
			{
				experiencePoints.bravery++;
			}
			else
			{
				moraleStateTicksRemaining = TICKS_PER_LOWMORALE_STATE;
				moraleState = (MoraleState)(randBoundsInclusive(state.rng, 1, 3));
				agent->modified_stats.morale += 15;
				stopAttacking();
				cancelMissions(state);
				aiList.reset(state, *this);
				// Notify
				switch (moraleState)
				{
					case MoraleState::PanicFreeze:
						sendAgentEvent(state, GameEventType::AgentFrozen, true);
						break;
					case MoraleState::PanicRun:
						sendAgentEvent(state, GameEventType::AgentPanicked, true);
						break;
					case MoraleState::Berserk:
						sendAgentEvent(state, GameEventType::AgentBerserk, true);
						break;
					default:
						break;
				}
				// Have a chance to drop items in hand
				if (moraleState != MoraleState::Berserk)
				{
					if (randBool(state.rng))
					{
						if (e1)
						{
							addMission(state, BattleUnitMission::dropItem(*this, e1));
						}
						if (e2)
						{
							addMission(state, BattleUnitMission::dropItem(*this, e2));
						}
					}
				}
			}
		}
	}
}

void BattleUnit::updateWoundsAndHealing(GameState &state, unsigned int ticks)
{
	auto e1 = agent->getFirstItemInSlot(EquipmentSlotType::LeftHand);
	auto e2 = agent->getFirstItemInSlot(EquipmentSlotType::RightHand);

	// Ensure still have item if healing
	if (isHealing)
	{
		isHealing = false;
		if (e1 && e1->type->type == AEquipmentType::Type::MediKit)
		{
			isHealing = true;
		}
		else if (e2 && e2->type->type == AEquipmentType::Type::MediKit)
		{
			isHealing = true;
		}
	}

	bool unconscious = isUnconscious();
	woundTicksAccumulated += ticks;
	while (woundTicksAccumulated >= TICKS_PER_WOUND_EFFECT)
	{
		woundTicksAccumulated -= TICKS_PER_WOUND_EFFECT;
		for (auto &w : fatalWounds)
		{
			if (w.second > 0)
			{
				agent->modified_stats.health -= w.second;
				if (isHealing && healingBodyPart == w.first)
				{
					w.second--;
					// healing fatal wound heals 5hp, as well as 1hp we just dealt in damage
					agent->modified_stats.health += 6;
					agent->modified_stats.health =
					    std::min(agent->modified_stats.health, agent->current_stats.health);
				}
			}
		}
	}
	// If fully healed the body part
	if (isHealing && fatalWounds[healingBodyPart] == 0)
	{
		isHealing = false;
	}
	// If died or went unconscious due to wounds
	if (isDead())
	{
		// FIXME: Should units dying to fatal wounds go out violently or non-violently?
		die(state);
	}
	if (!unconscious && isUnconscious())
	{
		fallUnconscious(state);
	}
}

void BattleUnit::updateRegen(GameState &state, unsigned int ticks)
{
	bool realTime = state.current_battle->mode == Battle::Mode::RealTime;

	regenTicksAccumulated += ticks;
	while (regenTicksAccumulated >= TICKS_PER_SECOND)
	{
		regenTicksAccumulated -= TICKS_PER_SECOND;
		// Stun removal
		if (stunDamage > 0)
		{
			stunDamage--;
		}
		if (!isConscious() && !isUnconscious() && !isDead())
		{
			tryToRiseUp(state);
		}
		// Psi regen
		if (agent->modified_stats.psi_energy < agent->current_stats.psi_energy)
		{
			agent->modified_stats.psi_energy++;
		}
		// Sta regen or expenditure RT
		if (realTime)
		{
			switch (current_movement_state)
			{
				// Regen if not moving
				case MovementState::None:
				{
					int staRegen = agent->current_stats.stamina >= 1920
					                   ? 30
					                   : (agent->current_stats.stamina >= 1280 ? 20 : 10);
					agent->modified_stats.stamina = std::min(
					    agent->modified_stats.stamina + staRegen, agent->current_stats.stamina);
				}
				break;
				// No expenditure for special movements
				case MovementState::Strafing:
				case MovementState::Brainsuck:
					break;
				// If Prone 1/sec
				case MovementState::Reverse:
				case MovementState::Normal:
					if (current_body_state != BodyState::Prone)
					{
						break;
					}
					if (agent->modified_stats.stamina > 10)
					{
						agent->modified_stats.stamina -= 10;
					}
					else
					{
						agent->modified_stats.stamina = 0;
					}
					break;
				// If Running 3/sec
				case MovementState::Running:
					if (agent->modified_stats.stamina > 30)
					{
						agent->modified_stats.stamina -= 30;
					}
					else
					{
						agent->modified_stats.stamina = 0;
					}
					break;
			}
		}
	}
	// Sta regen TB
	if (!realTime)
	{
		int staRegen = agent->current_stats.stamina >= 1920
		                   ? 60
		                   : (agent->current_stats.stamina >= 1280 ? 40 : 20);
		int tuLeft = 100 * agent->modified_stats.time_units / agent->current_stats.time_units;
		staRegen = tuLeft < 9 ? 0 : (tuLeft < 18 ? staRegen / 2 : staRegen);
		agent->modified_stats.stamina =
		    std::min(agent->modified_stats.stamina + staRegen, agent->current_stats.stamina);
	}
}

void BattleUnit::updateEvents(GameState &state)
{
	updateGiveWay(state);

	// Process spotting an enemy
	if (!visibleEnemies.empty())
	{
		// our target has a priority over others if enemy
		auto lastSeenEnemyPosition =
		    (targetUnit &&
		     state.current_battle->visibleEnemies[owner].find(targetUnit) != visibleEnemies.end())
		        ? targetUnit->position
		        : (*visibleEnemies.begin())->position;

		aiList.notifyEnemySpotted(lastSeenEnemyPosition);
	}
}

void BattleUnit::updateGiveWay(GameState &state)
{
	// Try giving way if asked to
	// FIXME: Ensure we're not in a firefight before giving way!
	if (giveWayRequestData.size() > 0)
	{
		if (!missions.empty() || !isConscious())
		{
			giveWayRequestData.clear();
		}
		else
		{
			// If we're given a giveWay request 0, 0 it means we're asked to kneel temporarily
			if (giveWayRequestData.size() == 1 && giveWayRequestData.front().x == 0 &&
			    giveWayRequestData.front().y == 0 &&
			    canAfford(state, getBodyStateChangeCost(target_body_state, BodyState::Kneeling)))
			{
				// Give way
				setMission(state, BattleUnitMission::changeStance(*this, BodyState::Kneeling));
				// Give time for that unit to pass
				addMission(state, BattleUnitMission::snooze(*this, TICKS_PER_SECOND), true);
			}
			else
			{
				auto from = tileObject->getOwningTile();
				auto helper = BattleUnitTileHelper{tileObject->map, *this};
				for (auto &newHeading : giveWayRequestData)
				{
					for (int z = -1; z <= 1; z++)
					{
						if (z < 0 || z >= tileObject->map.size.z)
						{
							continue;
						}
						// Try the new heading
						Vec3<int> pos = {position.x + newHeading.x, position.y + newHeading.y,
						                 position.z + z};

						if (!tileObject->map.isTileInBounds(pos))
							continue;

						auto to = tileObject->map.getTile(pos);
						// Check if heading on our level is acceptable
						bool acceptable =
						    helper.canEnterTile(from, to) && helper.canEnterTile(to, from);
						// If not, check if we can go down one tile
						if (!acceptable && pos.z - 1 >= 0)
						{
							pos -= Vec3<int>{0, 0, 1};
							to = tileObject->map.getTile(pos);
							acceptable =
							    helper.canEnterTile(from, to) && helper.canEnterTile(to, from);
						}
						// If not, check if we can go up one tile
						if (!acceptable && pos.z + 2 < tileObject->map.size.z)
						{
							pos += Vec3<int>{0, 0, 2};
							to = tileObject->map.getTile(pos);
							acceptable =
							    helper.canEnterTile(from, to) && helper.canEnterTile(to, from);
						}
						if (acceptable)
						{
							// 01: Give way (move 1 tile away)
							setMission(state, BattleUnitMission::gotoLocation(*this, pos, 0));
							// 02: Turn to previous facing
							addMission(state, BattleUnitMission::turn(*this, facing), true);
							// 03: Give time for that unit to pass
							addMission(state, BattleUnitMission::snooze(*this, 60), true);
							// 04: Return to our position after we're done
							addMission(state, BattleUnitMission::gotoLocation(*this, position, 0),
							           true);
							// 05: Turn to previous facing
							addMission(state, BattleUnitMission::turn(*this, facing), true);
						}
						if (!missions.empty())
						{
							break;
						}
					}
					if (!missions.empty())
					{
						break;
					}
				}
			}
			giveWayRequestData.clear();
		}
	}
}

void BattleUnit::updateIdling(GameState &state)
{
	bool realTime = state.current_battle->mode == Battle::Mode::RealTime;

	if (missions.empty() && isConscious())
	{
		// Sanity checks
		if (goalFacing != facing)
		{
			LogError("Unit %s (%s) turning without a mission, wtf?", id, agent->type->id);
		}
		if (target_body_state != current_body_state)
		{
			LogError("Unit %s (%s) changing body state without a mission, wtf?", id,
			         agent->type->id);
		}

		// Reach goal before everything else
		if (!atGoal)
		{
			// The only way unit can suddenly become not at goal when idling is if either
			// map part died under him, or dirt fell from above on him
			// Either way, moving to goal is not the correct way to solve this
			startFalling(state);
		}
		else if (realTime || (state.current_battle->interruptQueue.empty() &&
		                      (owner == state.current_battle->currentActiveOrganisation ||
		                       state.current_battle->interruptUnits.find({&state, id}) !=
		                           state.current_battle->interruptUnits.end())))
		{
			// Kneel if not kneeling and should kneel
			if (kneeling_mode == KneelingMode::Kneeling &&
			    current_body_state != BodyState::Kneeling && canKneel() &&
			    canAfford(state, getBodyStateChangeCost(target_body_state, BodyState::Kneeling),
			              true))
			{
				setMission(state, BattleUnitMission::changeStance(*this, BodyState::Kneeling));
			}
			// Go prone if not prone and should stay prone
			else if (movement_mode == MovementMode::Prone &&
			         current_body_state != BodyState::Prone &&
			         kneeling_mode != KneelingMode::Kneeling && canProne(position, facing) &&
			         canAfford(state, getBodyStateChangeCost(target_body_state, BodyState::Prone)))
			{
				setMission(state, BattleUnitMission::changeStance(*this, BodyState::Prone));
			}
			// Stand up if not standing up and should stand up
			else if ((movement_mode == MovementMode::Walking ||
			          movement_mode == MovementMode::Running) &&
			         kneeling_mode != KneelingMode::Kneeling &&
			         current_body_state != BodyState::Standing &&
			         current_body_state != BodyState::Flying)
			{
				if (agent->isBodyStateAllowed(BodyState::Standing))
				{
					if (canAfford(state,
					              getBodyStateChangeCost(target_body_state, BodyState::Standing)))
					{
						setMission(state,
						           BattleUnitMission::changeStance(*this, BodyState::Standing));
					}
				}
				else if (agent->isBodyStateAllowed(BodyState::Flying))
				{
					if (canAfford(state,
					              getBodyStateChangeCost(target_body_state, BodyState::Flying)))
					{
						setMission(state,
						           BattleUnitMission::changeStance(*this, BodyState::Flying));
					}
				}
				else
				{
					LogError("Hmm? Agent ordered to move walking without a standing/flying valid "
					         "body state?");
				}
			}
			// Stop flying if we can stand
			else if (current_body_state == BodyState::Flying &&
			         tileObject->getOwningTile()->getCanStand(isLarge()) &&
			         agent->isBodyStateAllowed(BodyState::Standing) &&
			         canAfford(state,
			                   getBodyStateChangeCost(target_body_state, BodyState::Standing)))
			{
				setMission(state, BattleUnitMission::changeStance(*this, BodyState::Standing));
			}
			// Stop being prone if legs are no longer supported and we haven't taken a mission yet
			if (current_body_state == BodyState::Prone && missions.empty() &&
			    agent->isBodyStateAllowed(BodyState::Kneeling))
			{
				bool hasSupport = true;
				for (auto &t : tileObject->occupiedTiles)
				{
					if (!tileObject->map.getTile(t)->getCanStand())
					{
						hasSupport = false;
						break;
					}
				}
				if (!hasSupport && canAfford(state, getBodyStateChangeCost(target_body_state,
				                                                           BodyState::Kneeling)))
				{
					setMission(state, BattleUnitMission::changeStance(*this, BodyState::Kneeling));
				}
			}
		}
	}
}

void BattleUnit::updateCrying(GameState &state)
{
	// FIXME: Implement proper crying
	if (!isConscious())
	{
		return;
	}
	// Our own units don't cry
	if (owner == state.current_battle->currentPlayer)
	{
		return;
	}
	// Agent doesn't know how to cry
	if (agent->type->crySfx.empty())
	{
		return;
	}
	// Crying timer works on real world time, not game time, so always decrement by 1
	ticksUntillNextCry -= 1;
	if (ticksUntillNextCry == 0)
	{
		resetCryTimer(state);
		// Cry chance in TB when it's not enemy's turn is 1/8 th
		if (state.current_battle->mode == Battle::Mode::TurnBased &&
		    owner != state.current_battle->currentActiveOrganisation &&
		    randBoundsInclusive(state.rng, 1, 8) == 1)
		{
			return;
		}
		// Actually cry
		fw().soundBackend->playSample(pickRandom(state.rng, agent->type->crySfx), getPosition());
	}
}

void BattleUnit::updateCheckBeginFalling(GameState &state)
{
	if (retreated)
	{
		return;
	}
	if (current_movement_state == MovementState::Brainsuck)
	{
		return;
	}
	if (!falling)
	{
		// Check if should fall or start flying
		if (!canFly() || current_body_state != BodyState::Flying)
		{
			bool hasSupport = false;
			bool fullySupported = true;
			if (tileObject->getOwningTile()->getCanStand(isLarge()))
			{
				hasSupport = true;
			}
			else
			{
				fullySupported = false;
			}
			if (!atGoal)
			{
				if (tileObject->map.getTile(goalPosition)->getCanStand(isLarge()))
				{
					hasSupport = true;
				}
				else
				{
					fullySupported = false;
				}
			}
			// If not flying and has no support - fall!
			if (!hasSupport && !canFly())
			{
				startFalling(state);
			}
			// If flying and not supported both on current and goal locations - start flying
			// Note: Throwing units can "hover" in standing body state
			if (!fullySupported && canFly() &&
			    (missions.empty() || missions.front()->type != BattleUnitMission::Type::ThrowItem))
			{
				if (current_body_state == target_body_state)
				{
					setBodyState(state, BodyState::Flying);
					if (!missions.empty())
					{
						missions.front()->targetBodyState = current_body_state;
					}
				}
			}
		}
	}
}

void BattleUnit::updateBody(GameState &state, unsigned int &bodyTicksRemaining)
{
	if (retreated)
	{
		return;
	}
	if (bodyTicksRemaining > 0)
	{
		if (body_animation_ticks_remaining > bodyTicksRemaining)
		{
			body_animation_ticks_remaining -= bodyTicksRemaining;
			bodyTicksRemaining = 0;
		}
		else
		{
			if (body_animation_ticks_remaining > 0)
			{
				bodyTicksRemaining -= body_animation_ticks_remaining;
				setBodyState(state, target_body_state);
			}
			// Pop finished missions if present
			popFinishedMissions(state);
			if (retreated)
			{
				return;
			}
			// Try to get new body state change
			// If hand state is changing, we can only interrupt "begin aiming"
			// Also, we cannot interrupt firing animation
			if (firing_animation_ticks_remaining == 0 &&
			    (hand_animation_ticks_remaining == 0 || target_hand_state == HandState::Aiming))
			{
				BodyState nextState = BodyState::Downed;
				if (getNextBodyState(state, nextState))
				{
					beginBodyStateChange(state, nextState);
				}
			}
		}
	}
}

void BattleUnit::updateMovementFalling(GameState &state, unsigned int &moveTicksRemaining, bool &)
{
	// Falling consumes remaining move ticks
	auto fallTicksRemaining = moveTicksRemaining / (agent->modified_stats.getMovementSpeed() *
	                                                BASE_MOVETICKS_CONSUMPTION_RATE);
	moveTicksRemaining = 0;

	if (collisionIgnoredTicks > 0)
	{
		if (collisionIgnoredTicks > fallTicksRemaining)
		{
			collisionIgnoredTicks -= fallTicksRemaining;
		}
		else
		{
			collisionIgnoredTicks = 0;
		}
	}

	auto previousPosition = position;
	auto newPosition = position;

	while (fallTicksRemaining-- > 0)
	{
		velocity.z -= FALLING_ACCELERATION_UNIT;
		newPosition += this->velocity / (float)TICK_SCALE / VELOCITY_SCALE_BATTLE;
	}

	// Check if new position is valid
	auto c = (collisionIgnoredTicks > 0 || isConscious())
	             ? Collision()
	             : tileObject->map.findCollision(previousPosition, newPosition, {}, tileObject);
	if (c)
	{
		// If colliding with anything but ground, bounce back once
		switch (c.obj->getType())
		{
			case TileObject::Type::Unit:
			case TileObject::Type::LeftWall:
			case TileObject::Type::RightWall:
			case TileObject::Type::Feature:
				if (!bounced)
				{
					if (velocity.x != 0.0f || velocity.y != 0.0f)
					{
						// If bounced do not try to find support this time
						bounced = true;
						newPosition = previousPosition;
						velocity.x = -velocity.x / 4;
						velocity.y = -velocity.y / 4;
						velocity.z = std::abs(velocity.z / 4);
					}
					else
					{
						bounced = true;
					}
					break;
				}
			// Intentional fall-through
			case TileObject::Type::Ground:
				// Let item fall so that it can collide with scenery or ground if falling on top of
				// it
				newPosition = {previousPosition.x, previousPosition.y,
				               std::min(newPosition.z, previousPosition.z)};
				break;
			default:
				LogError("What the hell is this unit colliding with? Type is %d",
				         (int)c.obj->getType());
				break;
		}
	}

	// Fell into a unit
	if (isConscious())
	{
		auto presentUnit =
		    tileObject->map.getTile(newPosition)->getUnitIfPresent(true, true, false, tileObject);
		if (presentUnit)
		{
			updateFallingIntoUnit(state, *presentUnit->getUnit());
		}
	}

	// If moved but did not find support - check if within level bounds and set position
	if (newPosition != previousPosition)
	{
		auto mapSize = this->tileObject->map.size;

		// Collision with ceiling
		if (newPosition.z >= mapSize.z)
		{
			newPosition.z = mapSize.z - 0.01f;
			velocity = {0.0f, 0.0f, 0.0f};
		}
		// Collision with map edge
		if (newPosition.x < 0 || newPosition.y < 0 || newPosition.y >= mapSize.y ||
		    newPosition.x >= mapSize.x || newPosition.y >= mapSize.y)
		{
			velocity.x = -velocity.x / 4;
			velocity.y = -velocity.y / 4;
			velocity.z = 0;
			newPosition = previousPosition;
		}
		// Fell below 0???
		if (newPosition.z < 0)
		{
			LogError("Unit at %f %f fell off the end of the world!?", newPosition.x, newPosition.y);
			die(state, nullptr, false);
			destroyed = true;
			return;
		}
		// Jump goal reached
		if (launched)
		{
			auto newGoalDist = newPosition - launchGoal;
			auto prevGoalDist = previousPosition - launchGoal;
			newGoalDist.z = 0.0f;
			prevGoalDist.z = 0.0f;
			if (glm::length(newGoalDist) > glm::length(prevGoalDist))
			{
				float extraVelocity = sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
				velocity.x = 0.0f;
				velocity.y = 0.0f;
				velocity.z -= extraVelocity / 2.0f / (float)VELOCITY_SCALE_BATTLE.x *
				              (float)VELOCITY_SCALE_BATTLE.z;
				launched = false;
			}
		}
		bool movedTiles = (Vec3<int>)position != (Vec3<int>)newPosition;
		setPosition(state, newPosition, movedTiles);
		triggerProximity(state);
	}

	// Falling units can always turn
	goalPosition = position;
	atGoal = true;

	// Check if reached ground
	if (collisionIgnoredTicks == 0)
	{
		auto restingPosition = tileObject->getOwningTile()->getRestingPosition(isLarge());
		if (position.z < restingPosition.z)
		{
			// Stopped falling
			falling = false;
			bounced = false;
			launched = false;
			collisionIgnoredTicks = 0;
			if (!isConscious())
			{
				// Bodies drop to the exact spot they fell upon
				setPosition(state, {position.x, position.y, restingPosition.z}, true);
			}
			else
			{
				// Conscious units drop to resting position
				setPosition(state, restingPosition, true);
			}
			resetGoal();
			// FIXME: Deal fall damage before nullifying this
			// FIXME: Play falling sound
			velocity = {0.0f, 0.0f, 0.0f};
		}
	}
}

void BattleUnit::updateFallingIntoUnit(GameState &state, BattleUnit &unit)
{
	if (agent->isBrainsucker)
	{
		// Must fall from above (almost)
		if (velocity.z < 0.1f)
		{
			if (position.z < unit.getMuzzleLocation().z)
			{
				StateRef<DamageType> brainsucker = {&state, "DAMAGETYPE_BRAINSUCKER"};
				if (!unit.brainSucker &&
				    brainsucker->dealDamage(100, unit.agent->type->damage_modifier) == 0)
				{
					// Cannot suck this head, get stunned
					applyDamageDirect(state, 9001, false, BodyPart::Body,
					                  agent->current_stats.health + TICKS_PER_TURN);
				}
				else
				{
					// Can suck this head, attach!
					int facingDelta = BattleUnitMission::getFacingDelta(facing, unit.facing);
					cancelMissions(state, true);
					spendRemainingTU(state, true);
					setMission(state,
					           BattleUnitMission::brainsuck(*this, {&state, unit.id}, facingDelta));
				}
			}
		}
	}
	else
	{
		// Get stunned
		applyDamageDirect(state, 9001, false, BodyPart::Body, agent->current_stats.health * 3 / 2);
	}
}

void BattleUnit::updateMovementNormal(GameState &state, unsigned int &moveTicksRemaining,
                                      bool &wasUsingLift)
{
	// We are not moving and not falling
	if (current_movement_state == MovementState::None)
	{
		// Check if we should adjust our current position
		if (goalPosition == getPosition())
		{
			goalPosition = tileObject->getOwningTile()->getRestingPosition(isLarge());
		}
		atGoal = goalPosition == getPosition();
		if (atGoal)
		{
			getNewGoal(state);
			if (retreated)
			{
				return;
			}
		}
	}
	// We are moving and not falling
	else
	{
		unsigned int speedModifier = 100;
		if (current_body_state == BodyState::Flying)
		{
			speedModifier = std::max((unsigned)1, flyingSpeedModifier);
		}
		Vec3<float> vectorToGoal = goalPosition - getPosition();
		unsigned int distanceToGoal = (unsigned)ceilf(glm::length(
		    vectorToGoal * VELOCITY_SCALE_BATTLE * (float)TICKS_PER_UNIT_TRAVELLED_BATTLEUNIT));
		unsigned int moveTicksConsumeRate = BASE_MOVETICKS_CONSUMPTION_RATE;
		// Bring all jumpers to constant speed so that we can align leg animation with the edge
		if (target_body_state == BodyState::Jumping)
		{
			moveTicksConsumeRate =
			    agent->modified_stats.getMovementSpeed() * BASE_MOVETICKS_CONSUMPTION_RATE / 15;
		}
		// Running is twice as fast
		else if (current_movement_state == MovementState::Running)
		{
			moveTicksConsumeRate = BASE_MOVETICKS_CONSUMPTION_RATE / 2;
		}
		// Crawling is at 66% speed
		else if (current_body_state == BodyState::Prone)
		{
			moveTicksConsumeRate = BASE_MOVETICKS_CONSUMPTION_RATE * 3 / 2;
		}

		// Quick check, if moving strictly vertical without falling or flying
		// then we must be using a lift.
		if (distanceToGoal > 0 && current_body_state != BodyState::Flying &&
		    current_movement_state != MovementState::Brainsuck && vectorToGoal.x == 0 &&
		    vectorToGoal.y == 0 && tileObject->getOwningTile()->hasLift)
		{
			// FIXME: Actually read set option
			if (config().getBool("OpenApoc.NewFeature.GravliftSounds") && !wasUsingLift)
			{
				playDistantSound(state, agent->type->gravLiftSfx, 0.25f);
			}
			usingLift = true;
			movement_ticks_passed = 0;
		}
		unsigned int movementTicksAccumulated = 0;
		// Cannot reach in one go
		if (distanceToGoal * moveTicksConsumeRate * 100 / speedModifier > moveTicksRemaining)
		{
			if (flyingSpeedModifier != 100)
			{
				flyingSpeedModifier = std::min(
				    (unsigned)100, flyingSpeedModifier + moveTicksRemaining / moveTicksConsumeRate /
				                                             FLYING_ACCELERATION_DIVISOR);
			}
			movementTicksAccumulated = moveTicksRemaining / moveTicksConsumeRate;
			auto dir = glm::normalize(vectorToGoal);
			Vec3<float> newPosition = (float)(moveTicksRemaining / moveTicksConsumeRate) *
			                          (float)(speedModifier / 100) * dir;
			newPosition /= VELOCITY_SCALE_BATTLE;
			newPosition /= (float)TICKS_PER_UNIT_TRAVELLED_BATTLEUNIT;
			newPosition += getPosition();
			setPosition(state, newPosition);
			triggerProximity(state);
			moveTicksRemaining = moveTicksRemaining % moveTicksConsumeRate;
			atGoal = false;
		}
		// Can reach in one go
		else
		{
			if (distanceToGoal > 0)
			{
				movementTicksAccumulated = distanceToGoal;
				if (flyingSpeedModifier != 100)
				{
					flyingSpeedModifier =
					    std::min((unsigned)100, flyingSpeedModifier +
					                                distanceToGoal / FLYING_ACCELERATION_DIVISOR);
				}
				moveTicksRemaining -= distanceToGoal * moveTicksConsumeRate;
				setPosition(state, goalPosition, true);
				triggerProximity(state);
				goalPosition = position;
			}
			getNewGoal(state);
			if (retreated)
			{
				return;
			}
		}

		// Scale ticks so that animations look proper on isometric screen
		// facing down or up on screen
		if (facing.x == facing.y)
		{
			movement_ticks_passed += movementTicksAccumulated * 100 / 150;
		}
		// facing left or right on screen
		else if (facing.x == -facing.y)
		{
			movement_ticks_passed += movementTicksAccumulated * 141 / 150;
		}
		else
		{
			movement_ticks_passed += movementTicksAccumulated;
		}
		// Footsteps sound
		if (shouldPlaySoundNow() && current_body_state != BodyState::Flying &&
		    current_movement_state != MovementState::Brainsuck)
		{
			playWalkSound(state);
		}
	}
}

void BattleUnit::updateMovementBrainsucker(GameState &state, unsigned int &moveTicksRemaining,
                                           bool &wasUsingLift)
{
	std::ignore = wasUsingLift;
	if (!missions.empty() && missions.front()->type == BattleUnitMission::Type::Brainsuck &&
	    getNextDestination(state, goalPosition))
	{
		// Just increment ticks passed to play animation
		movement_ticks_passed += moveTicksRemaining / BASE_MOVETICKS_CONSUMPTION_RATE;
		moveTicksRemaining = 0;
		bool movedTiles = (Vec3<int>)position != (Vec3<int>)goalPosition;
		setPosition(state, goalPosition, movedTiles);
		atGoal = true;
	}
	else
	{
		resetGoal();
		startFalling(state);
	}
}

void BattleUnit::updateMovementJumping(GameState &state, unsigned int &moveTicksRemaining,
                                       bool &wasUsingLift)
{
	std::ignore = wasUsingLift;
	// Check if jump is complete
	if (launched)
	{
		// Jump complete, await body state change
		if (position.z <= goalPosition.z && velocity.z < 0.0f)
		{
			getNewGoal(state);
			return;
		}
	}
	// Launch us in the air
	else
	{
		launched = true;
		Vec3<float> targetVector = goalPosition - position;
		Vec3<float> targetVectorXY = {targetVector.x, targetVector.y, 0.0f};
		float distance = glm::length(targetVectorXY);
		float velocityXY;
		float velocityZ;
		calculateVelocityForJump(distance, position.z - goalPosition.z, velocityXY, velocityZ,
		                         distance > 1.0f);
		velocity =
		    (glm::normalize(targetVectorXY) * velocityXY + Vec3<float>{0.0f, 0.0f, velocityZ}) *
		    VELOCITY_SCALE_BATTLE;
	}

	auto jumpTicksRemaining = moveTicksRemaining / (agent->modified_stats.getMovementSpeed() *
	                                                BASE_MOVETICKS_CONSUMPTION_RATE);
	moveTicksRemaining = 0;
	auto newPosition = position;
	while (jumpTicksRemaining-- > 0)
	{
		velocity.z -= FALLING_ACCELERATION_UNIT;
		newPosition += this->velocity / (float)TICK_SCALE / VELOCITY_SCALE_BATTLE;
	}
	setPosition(state, newPosition);
}

void BattleUnit::updateMovement(GameState &state, unsigned int &moveTicksRemaining,
                                bool &wasUsingLift)
{
	if (retreated)
	{
		return;
	}
	if (moveTicksRemaining > 0)
	{
		// Turn off Jetpacks
		if (current_body_state != BodyState::Flying)
		{
			flyingSpeedModifier = 0;
		}

		// If falling then process falling
		if (falling)
		{
			return updateMovementFalling(state, moveTicksRemaining, wasUsingLift);
		}
		// If brainsucking simply stay on head
		else if (current_movement_state == MovementState::Brainsuck)
		{
			return updateMovementBrainsucker(state, moveTicksRemaining, wasUsingLift);
		}
		else if (current_movement_state != MovementState::None &&
		         (current_body_state == BodyState::Jumping &&
		          target_body_state == BodyState::Jumping))
		{
			return updateMovementJumping(state, moveTicksRemaining, wasUsingLift);
		}
		else
		{
			return updateMovementNormal(state, moveTicksRemaining, wasUsingLift);
		}
	}
}

void BattleUnit::updateHands(GameState &, unsigned int &handsTicksRemaining)
{
	if (retreated)
	{
		return;
	}
	if (handsTicksRemaining > 0)
	{
		if (firing_animation_ticks_remaining > 0)
		{
			if (firing_animation_ticks_remaining > handsTicksRemaining)
			{
				firing_animation_ticks_remaining -= handsTicksRemaining;
				handsTicksRemaining = 0;
			}
			else
			{
				handsTicksRemaining -= firing_animation_ticks_remaining;
				firing_animation_ticks_remaining = 0;
				if (canHandStateChange(HandState::Aiming))
				{
					setHandState(HandState::Aiming);
				}
				else
				{
					setHandState(HandState::AtEase);
				}
			}
		}
		else
		{
			if (hand_animation_ticks_remaining > handsTicksRemaining)
			{
				hand_animation_ticks_remaining -= handsTicksRemaining;
				handsTicksRemaining = 0;
			}
			else
			{
				if (hand_animation_ticks_remaining > 0)
				{
					handsTicksRemaining -= hand_animation_ticks_remaining;
					hand_animation_ticks_remaining = 0;
					setHandState(target_hand_state);
				}
			}
		}
	}
}

void BattleUnit::updateTurning(GameState &state, unsigned int &turnTicksRemaining,
                               unsigned int const handsTicksRemaining)
{
	if (retreated)
	{
		return;
	}
	if (turnTicksRemaining > 0)
	{
		// If firing then consume turning ticks since we can't turn while firing
		if (firing_animation_ticks_remaining > 0)
		{
			// If firing animation will be finished this time, then subtract
			// the amount of ticks required to finish it
			// Otherwise subtract all ticks
			turnTicksRemaining -= handsTicksRemaining >= firing_animation_ticks_remaining
			                          ? firing_animation_ticks_remaining
			                          : turnTicksRemaining;
		}
		if (turning_animation_ticks_remaining > turnTicksRemaining)
		{
			turning_animation_ticks_remaining -= turnTicksRemaining;
			turnTicksRemaining = 0;
		}
		else
		{
			if (turning_animation_ticks_remaining > 0)
			{
				turnTicksRemaining -= turning_animation_ticks_remaining;
				setFacing(state, goalFacing);
			}
			// Pop finished missions if present
			popFinishedMissions(state);
			if (retreated)
			{
				return;
			}
			// Try to get new facing change
			Vec2<int> nextFacing;
			if (getNextFacing(state, nextFacing))
			{
				if (current_movement_state == MovementState::Brainsuck)
				{
					setFacing(state, nextFacing);
				}
				else
				{
					beginTurning(state, nextFacing);
				}
			}
		}
	}
}

void BattleUnit::updateDisplayedItem(GameState &state)
{
	if (!agent->type->inventory)
	{
		return;
	}
	auto lastDisplayedItem = displayedItem;
	bool foundThrownItem = false;
	if (missions.size() > 0)
	{
		for (auto &m : missions)
		{
			if (m->type != BattleUnitMission::Type::ThrowItem || !m->item)
			{
				continue;
			}
			displayedItem = m->item->type;
			foundThrownItem = true;
			break;
		}
	}
	if (!foundThrownItem)
	{
		// If we're firing - try to keep last displayed item same, even if not dominant
		displayedItem = agent->getDominantItemInHands(
		    state, firing_animation_ticks_remaining > 0 ? lastDisplayedItem : nullptr);
	}
	// If displayed item changed or we are throwing - bring hands into "AtEase" state immediately
	if (foundThrownItem || displayedItem != lastDisplayedItem)
	{
		if (hand_animation_ticks_remaining > 0 || current_hand_state != HandState::AtEase)
		{
			setHandState(HandState::AtEase);
		}
	}
	if (displayedItem != lastDisplayedItem)
	{
		refreshReserveCost(state);
	}
}

bool BattleUnit::updateAttackingRunCanFireChecks(GameState &state, unsigned int ticks,
                                                 sp<AEquipment> &weaponRight,
                                                 sp<AEquipment> &weaponLeft,
                                                 Vec3<float> &targetPosition)
{
	// Decrement target check timer if we aim at a unit
	if (targetingMode == TargetingMode::Unit)
	{
		if (ticksUntillNextTargetCheck > ticks)
		{
			ticksUntillNextTargetCheck -= ticks;
		}
		else
		{
			ticksUntillNextTargetCheck = 0;
		}
	}

	// We cannot fire if we have no weapon capable of firing
	if (!weaponLeft && !weaponRight)
	{
		return false;
	}

	// We cannot fire if it's time to check target unit and it fell unconscious or died
	// or is unavailable (not in sight or no LOF) for a long time
	// Also, at this point we will turn to target tile if targeting tile
	// and track (turn to) target unit if it's visible

	// Note:
	// - If target is a unit, this check is done regularly
	// - If target is a tile, this check will only be done once after start,
	//   and again once each time this unit stops moving
	if (ticksUntillNextTargetCheck == 0)
	{
		ticksUntillNextTargetCheck = LOS_CHECK_INTERVAL_TRACKING;
		// If targeting unit check if up and track it if we see it
		if (targetingMode == TargetingMode::Unit)
		{
			// Stop firing if target went unconscious
			if (!targetUnit->isConscious()
			    // Stop firing if target became friendly and we're not berserk
			    || (moraleState != MoraleState::Berserk &&
			        owner->isRelatedTo(targetUnit->owner) != Organisation::Relation::Hostile))
			{
				return false;
			}
			// "timesTargetMIA" is set to 1 when ready to fire but target is not in sight or no LOF
			// If it was then here we will check if target is still MIA
			// Target is MIA if it's either not in sight or has no LOF to it
			// If target is MIA for too long we will cancel the attack
			bool targetMIA = timesTargetMIA > 0;
			if (state.current_battle->visibleEnemies[owner].find(targetUnit) !=
			    state.current_battle->visibleEnemies[owner].end())
			{
				targetTile = targetUnit->position;
				targetMIA = targetMIA && !hasLineToUnit(targetUnit);
			}
			if (targetMIA)
			{
				timesTargetMIA++;
				if (timesTargetMIA > TIMES_TO_WAIT_FOR_MIA_TARGET)
				{
					return false;
				}
			}
			else
			{
				timesTargetMIA = 0;
			}
		}
		// Check if we are in range
		if (weaponRight && !weaponRight->canFire(state, targetPosition))
		{
			weaponRight = nullptr;
		}
		if (weaponLeft && !weaponLeft->canFire(state, targetPosition))
		{
			weaponLeft = nullptr;
		}
		// We cannot fire if both weapons are out of range
		if (!weaponLeft && !weaponRight)
		{
			return false;
		}
		// Check if we should turn to target tile (only do this if we're stationary)
		if (current_movement_state == MovementState::None)
		{
			if (BattleUnitMission::getFacing(*this, targetTile) != facing)
			{
				addMission(state, BattleUnitMission::turn(*this, targetTile));
			}
		}
	}
	// Even if it's not time we must check if ready weapons can fire
	else
	{
		if (weaponRight && weaponRight->readyToFire && !weaponRight->canFire(state, targetPosition))
		{
			// Introduce small delay so we're not re-checking every frame
			weaponRight->weapon_fire_ticks_remaining += WEAPON_MISFIRE_DELAY_TICKS;
			weaponRight = nullptr;
		}
		if (weaponLeft && weaponLeft->readyToFire && !weaponLeft->canFire(state, targetPosition))
		{
			weaponLeft->weapon_fire_ticks_remaining += WEAPON_MISFIRE_DELAY_TICKS;
			weaponLeft = nullptr;
		}
	}
	return true;
}

void BattleUnit::updateAttacking(GameState &state, unsigned int ticks)
{
	if (isAttacking())
	{
		// Cancel acquire TU mission if attempting to attack
		if (!missions.empty() && missions.front()->type == BattleUnitMission::Type::AcquireTU)
		{
			cancelMissions(state);
		}
		// Prepare all needed positions
		static const Vec3<float> offsetTile = {0.5f, 0.5f, 0.0f};
		static const Vec3<float> offsetTileGround = {0.5f, 0.5f, 10.0f / 40.0f};
		Vec3<float> muzzleLocation = getMuzzleLocation();
		Vec3<float> targetPosition;
		switch (targetingMode)
		{
			case TargetingMode::Unit:
				targetPosition = targetUnit->tileObject->getVoxelCentrePosition();
				break;
			case TargetingMode::TileCenter:
			{
				// Shoot parallel to the ground
				float unitZ = muzzleLocation.z;
				unitZ -= (int)unitZ;
				targetPosition =
				    (Vec3<float>)targetTile + offsetTile + Vec3<float>{0.0f, 0.0f, unitZ};
				break;
			}
			case TargetingMode::TileGround:
				targetPosition = (Vec3<float>)targetTile + offsetTileGround;
				break;
			case TargetingMode::NoTarget:
				LogError("Invalid targeting mode NoTarget while targeting");
				break;
		}

		// Prepare weapons we can use
		// We can use a weapon if we're set to fire this hand, and it's a weapon that can be fired
		auto weaponRight = agent->getFirstItemInSlot(EquipmentSlotType::RightHand);
		auto weaponLeft = agent->getFirstItemInSlot(EquipmentSlotType::LeftHand);
		switch (weaponStatus)
		{
			case WeaponStatus::FiringBothHands:
				if (weaponRight && weaponRight->needsReload())
				{
					weaponRight->loadAmmo(state);
				}
				if (weaponRight && !weaponRight->canFire(state))
				{
					weaponRight = nullptr;
				}
				if (weaponLeft && weaponLeft->needsReload())
				{
					weaponLeft->loadAmmo(state);
				}
				if (weaponLeft && !weaponLeft->canFire(state))
				{
					weaponLeft = nullptr;
				}
				break;
			case WeaponStatus::FiringRightHand:
				if (weaponRight && weaponRight->needsReload())
				{
					weaponRight->loadAmmo(state);
				}
				if (weaponRight && !weaponRight->canFire(state))
				{
					weaponRight = nullptr;
				}
				weaponLeft = nullptr;
				break;
			case WeaponStatus::FiringLeftHand:
				if (weaponLeft && weaponLeft->needsReload())
				{
					weaponLeft->loadAmmo(state);
				}
				if (weaponLeft && !weaponLeft->canFire(state))
				{
					weaponLeft = nullptr;
				}
				weaponRight = nullptr;
				break;
			case WeaponStatus::NotFiring:
				LogError("Invalid targeting mode NoTarget while targeting");
				break;
		}

		// Firing - Make checks if we should stop firing, confirm target, turn to it etc.
		if (!updateAttackingRunCanFireChecks(state, ticks, weaponRight, weaponLeft, targetPosition))
		{
			stopAttacking();
		}
		else
		{
			updateFiring(state, weaponLeft, weaponRight, targetPosition);
		}
	}

	// Not attacking:
	// - was not firing
	// - was firing but fire checks just failed
	// - just fired and stopped firing
	if (!isAttacking())
	{
		// Decrement timer for residual aiming
		if (residual_aiming_ticks_remaining > 0)
		{
			residual_aiming_ticks_remaining -= ticks;
		}
		else if (canHandStateChange(HandState::AtEase))
		{
			beginHandStateChange(HandState::AtEase);
		}
	}
}

void BattleUnit::updateFiring(GameState &state, sp<AEquipment> &weaponLeft,
                              sp<AEquipment> &weaponRight, Vec3<float> &targetPosition)
{
	bool realTime = state.current_battle->mode == Battle::Mode::RealTime;
	Vec3<float> muzzleLocation = getMuzzleLocation();

	// Should we start aiming?
	if (canHandStateChange(HandState::Aiming))
	{
		beginHandStateChange(HandState::Aiming);
	}

	// Should we begin fire delay countdown?
	if (target_hand_state == HandState::Aiming)
	{
		if (weaponRight && !weaponRight->isFiring())
		{
			weaponRight->startFiring(fire_aiming_mode, !realTime);
		}
		if (weaponLeft && !weaponLeft->isFiring())
		{
			weaponLeft->startFiring(fire_aiming_mode, !realTime);
		}
	}

	// Is a gun ready to fire now?
	bool weaponFired = false;
	if (firing_animation_ticks_remaining == 0 && target_hand_state == HandState::Aiming)
	{
		sp<AEquipment> firingWeapon = nullptr;
		if (weaponRight && weaponRight->readyToFire)
		{
			firingWeapon = weaponRight;
			weaponRight = nullptr;
		}
		else if (weaponLeft && weaponLeft->readyToFire)
		{
			firingWeapon = weaponLeft;
			weaponLeft = nullptr;
		}
		// Weapon ready to fire: check if facing the right way
		if (firingWeapon)
		{
			auto targetPosAdjusted = targetPosition;
			// Lead the target
			if (targetUnit)
			{
				auto projectileVelocity =
				    firingWeapon->getPayloadType()->speed * PROJECTILE_VELOCITY_MULTIPLIER;
				// Target's velocity (if falling/jumping)
				Vec3<float> targetVelocity = targetUnit->getVelocity();
				auto distanceVoxels =
				    glm::length((targetPosition - muzzleLocation) * VELOCITY_SCALE_BATTLE);
				float timeToImpact = distanceVoxels * (float)TICK_SCALE / projectileVelocity;
				targetPosAdjusted += Collision::getLeadingOffset(targetPosition - muzzleLocation,
				                                                 projectileVelocity * timeToImpact,
				                                                 targetVelocity * timeToImpact);
			}

			auto targetVector = targetPosAdjusted - muzzleLocation;
			targetVector = {targetVector.x, targetVector.y, 0.0f};
			// If we meddled with target position see that we didn't break stuff
			if (targetPosAdjusted != targetPosition)
			{
				// Target must be within frontal arc, must have LOF to target
				if (glm::angle(glm::normalize(targetVector),
				               glm::normalize(Vec3<float>{facing.x, facing.y, 0})) >= M_PI / 2 ||
				    !hasLineToPosition(targetPosAdjusted))
				{
					// Try firing at unit itself if we can't fire at the leading point
					targetPosAdjusted = targetPosition;
					targetVector = targetPosAdjusted - muzzleLocation;
					targetVector = {targetVector.x, targetVector.y, 0.0f};
				}
			}

			// Target must be within frontal arc, must have LOF to if unit
			if (glm::angle(glm::normalize(targetVector),
			               glm::normalize(Vec3<float>{facing.x, facing.y, 0})) < M_PI / 2)
			{
				if (targetingMode == TargetingMode::Unit &&
				    (state.current_battle->visibleEnemies[owner].find(targetUnit) ==
				         state.current_battle->visibleEnemies[owner].end() ||
				     !hasLineToUnit(targetUnit)))
				{
					// No LOF to target unit
					// Raise targetMIA flag, so that we start tracking how long is it MIA
					if (timesTargetMIA == 0)
					{
						timesTargetMIA = 1;
					}
				}
				else if (realTime || spendTU(state, getAttackCost(state, *firingWeapon, targetTile),
				                             true, true, true))
				{
					// Can fire and can afford firing, FIRE!
					firingWeapon->fire(state, targetPosAdjusted,
					                   targetingMode == TargetingMode::Unit ? targetUnit : nullptr);
					cloakTicksAccumulated = 0;
					if (agent->type->inventory)
					{
						displayedItem = firingWeapon->type;
					}
					setHandState(HandState::Firing);
					weaponFired = true;
				}
				else
				{
					// Can't afford firing
					stopAttacking();
				}
			}
			else
			{
				// Target not in frontal arc
				// If this is a unit, start tracking how long is it so
				if (targetingMode == TargetingMode::Unit)
				{
					// Raise targetMIA flag, so that we start tracking how long is it MIA
					if (timesTargetMIA == 0)
					{
						timesTargetMIA = 1;
					}
				}
			}
		}
	}

	// Cancel firing (unless zerking):
	// - If turn based
	// - If fired weapon at ground - stop firing that hand
	if (weaponFired && moraleState != MoraleState::Berserk &&
	    (!realTime || targetingMode != TargetingMode::Unit))
	{
		switch (weaponStatus)
		{
			case WeaponStatus::FiringBothHands:
				if (!weaponRight)
				{
					if (!weaponLeft)
					{
						stopAttacking();
					}
					else
					{
						weaponStatus = WeaponStatus::FiringLeftHand;
					}
				}
				else if (!weaponLeft)
				{
					weaponStatus = WeaponStatus::FiringRightHand;
				}
				break;
			case WeaponStatus::FiringLeftHand:
				if (!weaponLeft)
				{
					stopAttacking();
				}
				break;
			case WeaponStatus::FiringRightHand:
				if (!weaponRight)
				{
					stopAttacking();
				}
				break;
			case WeaponStatus::NotFiring:
				LogError("Weapon fired while not firing!?");
				break;
		}
	}
}
void BattleUnit::updatePsi(GameState &state, unsigned int ticks)
{
	bool realTime = state.current_battle->mode == Battle::Mode::RealTime;
	if (psiStatus != PsiStatus::NotEngaged)
	{
		auto e1 = agent->getFirstItemInSlot(EquipmentSlotType::RightHand);
		auto e2 = agent->getFirstItemInSlot(EquipmentSlotType::LeftHand);
		if (e1 && e1->type != psiItem)
		{
			e1 = nullptr;
		}
		if (e2 && e2->type != psiItem)
		{
			e2 = nullptr;
		}
		auto bender = e1 ? e1 : e2;
		if (!bender)
		{
			stopAttackPsi(state);
		}
		else if (realTime)
		{
			ticksAccumulatedToNextPsiCheck += ticks;
			while (ticksAccumulatedToNextPsiCheck >= TICKS_PER_PSI_CHECK)
			{
				ticksAccumulatedToNextPsiCheck -= TICKS_PER_PSI_CHECK;
				auto cost = getPsiCost(psiStatus, false);
				if (cost > agent->modified_stats.psi_energy)
				{
					stopAttackPsi(state);
				}
				else
				{
					agent->modified_stats.psi_energy -= cost;
					psiTarget->applyPsiAttack(state, *this, psiStatus, psiItem, false);
				}
			}
		}
	}
}

void BattleUnit::updateAI(GameState &state, unsigned int)
{
	if (!isConscious())
	{
		return;
	}

	// Decide
	auto decision = aiList.think(state, *this);
	if (!decision.isEmpty())
	{
		LogWarning("AI %s for unit %s decided to %s", decision.ai, id, decision.getName());
		executeAIDecision(state, decision);
	}
}

void BattleUnit::triggerProximity(GameState &state)
{
	auto it = state.current_battle->items.begin();
	while (it != state.current_battle->items.end())
	{
		auto i = *it++;
		if (!i->item->primed || i->item->triggerDelay > 0)
		{
			continue;
		}
		// Proximity explosion trigger
		if ((i->item->triggerType == TriggerType::Proximity ||
		     i->item->triggerType == TriggerType::Boomeroid) &&
		    BattleUnitTileHelper::getDistanceStatic(position, i->position) <=
		        i->item->triggerRange &&
		    i->item->getPayloadType()->damage_type->effectType !=
		        DamageType::EffectType::Brainsucker)
		{
			i->die(state);
		}
		// Boomeroid hopping trigger
		else if (i->item->triggerType == TriggerType::Boomeroid &&
		         BattleUnitTileHelper::getDistanceStatic(position, i->position) <= BOOMEROID_RANGE)
		{
			i->hopTo(state, position);
		}
	}
}

void BattleUnit::triggerBrainsuckers(GameState &state)
{
	StateRef<DamageType> brainsucker = {&state, "DAMAGETYPE_BRAINSUCKER"};
	if (brainsucker->dealDamage(100, agent->type->damage_modifier) == 0)
		return;

	auto it = state.current_battle->items.begin();
	while (it != state.current_battle->items.end())
	{
		auto i = *it++;
		if (!i->item->primed || i->item->triggerDelay > 0)
		{
			continue;
		}
		if (i->item->triggerType == TriggerType::Proximity &&
		    i->item->getPayloadType()->damage_type->effectType ==
		        DamageType::EffectType::Brainsucker &&
		    BattleUnitTileHelper::getDistanceStatic(position, i->position) <= i->item->triggerRange)
		{
			auto aliens = state.getAliens();
			if (aliens->isRelatedTo(owner) == Organisation::Relation::Hostile)
			{
				// Sound for hatching
				if (state.battle_common_sample_list->brainsuckerHatch)
				{
					fw().soundBackend->playSample(state.battle_common_sample_list->brainsuckerHatch,
					                              position);
				}
				state.current_battle->spawnUnit(state, aliens, {&state, "AGENTTYPE_BRAINSUCKER"},
				                                i->position, {0, 1}, BodyState::Throwing);
				i->die(state, false);
			}
		}
	}
}

void BattleUnit::startFalling(GameState &state)
{
	if (target_body_state == BodyState::Jumping)
	{
		setBodyState(state, BodyState::Standing);
	}
	setMovementState(MovementState::None);
	falling = true;
	launched = false;
}

bool BattleUnit::calculateVelocityForLaunch(float distanceXY, float diffZ, float &velocityXY,
                                            float &velocityZ, float initialXY)
{
	static float dZ = 0.1f;

	// Initial setup
	velocityXY = initialXY;

	float a = -FALLING_ACCELERATION_UNIT / VELOCITY_SCALE_BATTLE.z / 2.0f / TICK_SCALE;
	float c = diffZ;
	float t = 0.0f;

	// We will continue reducing velocityXY  until we find such a trajectory
	// that makes the agent fall on top of the tile
	while (velocityXY > 0.0f)
	{
		t = distanceXY * TICK_SCALE / (velocityXY);
		velocityZ = TICK_SCALE * ((-c - a * t * t) / t - a);
		if (velocityZ - (FALLING_ACCELERATION_UNIT / VELOCITY_SCALE_BATTLE.z) * t < -0.125f)
		{
			return true;
		}
		else
		{
			velocityXY -= dZ;
		}
	}
	return false;
}

void BattleUnit::calculateVelocityForJump(float distanceXY, float diffZ, float &velocityXY,
                                          float &velocityZ, bool diagonAlley)
{
	velocityXY = diagonAlley ? 0.75f : 0.5f;

	float a = -FALLING_ACCELERATION_UNIT / VELOCITY_SCALE_BATTLE.z / 2.0f / TICK_SCALE;
	float c = diffZ;
	float t = 0.0f;

	t = distanceXY * TICK_SCALE / (velocityXY);
	velocityZ = TICK_SCALE * ((-c - a * t * t) / t - a);
	return;
}

bool BattleUnit::canLaunch(Vec3<float> targetPosition)
{
	Vec3<float> targetVectorXY;
	float velocityXY;
	float velocityZ;
	return canLaunch(targetPosition, targetVectorXY, velocityXY, velocityZ);
}

bool BattleUnit::canLaunch(Vec3<float> targetPosition, Vec3<float> &targetVectorXY,
                           float &velocityXY, float &velocityZ)
{
	// Flying units cannot jump
	if (canFly())
	{
		return false;
	}
	Vec3<float> targetVector = targetPosition - position;
	// Cannot jump to the same XY
	if (targetVector.x == 0.0f && targetVector.y == 0.0f)
	{
		return false;
	}
	// Cannot jump if target too far away and we are not a sucker
	Vec3<int> posDiff = (Vec3<int>)position - (Vec3<int>)targetPosition;
	bool sucker = agent->isBrainsucker;
	int limit = sucker ? 5 : 1;
	if (std::abs(posDiff.x) > limit || std::abs(posDiff.y) > limit || std::abs(posDiff.z) > 1)
	{
		return false;
	}
	// Calculate starting velocity
	targetVectorXY = {targetVector.x, targetVector.y, 0.0f};
	float distance = glm::length(targetVectorXY);
	float initialXY = 0.5f;
	if (sucker)
	{
		initialXY *= sqrtf(targetVector.x * targetVector.x + targetVector.y * targetVector.y);
	}
	if (!calculateVelocityForLaunch(distance, position.z - targetPosition.z, velocityXY, velocityZ,
	                                initialXY))
	{
		return false;
	}
	return true;
}

void BattleUnit::launch(GameState &state, Vec3<float> targetPosition, BodyState bodyState)
{
	Vec3<float> targetVectorXY;
	float velocityXY;
	float velocityZ;
	if (!canLaunch(targetPosition, targetVectorXY, velocityXY, velocityZ))
	{
		return;
	}
	startFalling(state);
	launchGoal = targetPosition;
	launched = true;
	velocity = (glm::normalize(targetVectorXY) * velocityXY + Vec3<float>{0.0f, 0.0f, velocityZ}) *
	           VELOCITY_SCALE_BATTLE;
	collisionIgnoredTicks = (int)ceilf(36.0f / glm::length(velocity / VELOCITY_SCALE_BATTLE)) + 1;
	beginBodyStateChange(state, bodyState);
}

void BattleUnit::startMoving(GameState &state)
{
	auto targetMovementMode = missions.empty()
	                              ? MovementState::None
	                              : missions.front()->getNextMovementState(state, *this);
	if (targetMovementMode == MovementState::None)
	{
		if (movement_mode == MovementMode::Running && current_body_state != BodyState::Prone)
		{
			targetMovementMode = MovementState::Running;
		}
		else
		{
			targetMovementMode = MovementState::Normal;
		}
	}
	setMovementState(targetMovementMode);
}

void BattleUnit::requestGiveWay(const BattleUnit &requestor,
                                const std::list<Vec3<int>> &plannedPath, Vec3<int> pos)
{
	// If asked already or busy - cannot give way
	if (!giveWayRequestData.empty() || isBusy())
	{
		return;
	}
	// If unit is prone and we're trying to go into it's legs
	if (current_body_state == BodyState::Prone && tileObject->getOwningTile()->position != pos)
	{
		// Just ask unit to kneel for a moment
		giveWayRequestData.emplace_back(0, 0);
	}
	// If unit is not prone or we're trying to go into it's body
	else
	{
		// Start with unit's facing, and go to the sides, adding facings
		// if they're not in our path and not our current position.
		// Next facings: [0] is clockwise, [1] is counter-clockwise from current
		std::vector<int> nextFacings = {facing_dir_map.at(facing), facing_dir_map.at(facing)};
		for (int i = 0; i <= 4; i++)
		{
			int limit = i == 0 || i == 4 ? 0 : 1;
			for (int j = 0; j <= limit; j++)
			{
				auto nextFacing = dir_facing_map.at(nextFacings[j]);
				Vec3<int> nextPos = {position.x + nextFacing.x, position.y + nextFacing.y,
				                     position.z};
				if (nextPos == (Vec3<int>)requestor.position ||
				    std::find(plannedPath.begin(), plannedPath.end(), nextPos) != plannedPath.end())
				{
					continue;
				}
				bool inLargePath = false;
				if (requestor.isLarge())
				{
					for (int x = 0; x <= 1; x++)
					{
						for (int y = 0; y <= 1; y++)
						{
							for (int z = 0; z <= 1; z++)
							{
								if (x == 0 && y == 0 && z == 0)
								{
									continue;
								}
								if (std::find(plannedPath.begin(), plannedPath.end(),
								              nextPos - Vec3<int>{x, y, z}) != plannedPath.end())
								{
									inLargePath = true;
									break;
								}
							}
							if (inLargePath)
							{
								break;
							}
						}
						if (inLargePath)
						{
							break;
						}
					}
				}
				if (inLargePath)
				{
					continue;
				}
				giveWayRequestData.push_back(nextFacing);
			}
			nextFacings[0] = nextFacings[0] == 7 ? 0 : nextFacings[0] + 1;
			nextFacings[1] = nextFacings[1] == 0 ? 7 : nextFacings[1] - 1;
		}
	}
}

void BattleUnit::applyEnzymeEffect(GameState &state)
{
	spawnEnzymeSmoke(state);

	// Damage random item
	if (agent->type->inventory && !agent->equipment.empty())
	{
		auto item = pickRandom(state.rng, agent->equipment);
		item->armor -= enzymeDebuffIntensity / 2;

		// Item destroyed
		if (item->armor <= 0)
		{
			if (item->type->type == AEquipmentType::Type::Grenade)
			{
				item->explode(state);
			}
			else
			{
				agent->removeEquipment(state, item);
			}
		}
	}

	// Finally, reduce debuff

	enzymeDebuffIntensity--;
}

void BattleUnit::spawnEnzymeSmoke(GameState &state)
{
	// FIXME: Ensure this is proper, for now just emulating vanilla crudely
	// This makes smoke spawned by enzyme grow smaller when debuff runs out
	int divisor = std::max(1, 36 / enzymeDebuffIntensity);
	StateRef<DamageType> smokeDamageType = {&state, "DAMAGETYPE_SMOKE"};
	// Power of 0 means no spread
	state.current_battle->placeHazard(state, owner, {&state, id}, smokeDamageType, position,
	                                  smokeDamageType->hazardType->getLifetime(state), 0, divisor,
	                                  false);
}

void BattleUnit::sendAgentEvent(GameState &state, GameEventType type, bool checkOwnership,
                                bool checkVisibility) const
{
	if ((!checkOwnership ||
	     (owner == state.current_battle->currentPlayer && agent->type->allowsDirectControl)) &&
	    (!checkVisibility || owner == state.current_battle->currentPlayer ||
	     state.current_battle->visibleUnits[state.current_battle->currentPlayer].find(
	         {&state, id}) !=
	         state.current_battle->visibleUnits[state.current_battle->currentPlayer].end()))
	{
		fw().pushEvent(new GameAgentEvent(type, {&state, agent.id}));
	}
}

int BattleUnit::rollForPrimaryStat(GameState &state, int experience)
{
	if (experience > 10)
	{
		return randBoundsInclusive(state.rng, 2, 6);
	}
	else if (experience > 5)
	{
		return randBoundsInclusive(state.rng, 1, 4);
	}
	if (experience > 2)
	{
		return randBoundsInclusive(state.rng, 1, 3);
	}
	if (experience > 0)
	{
		return randBoundsInclusive(state.rng, 0, 1);
	}
	return 0;
}

// FIXME: Ensure correct
// For now, using X-Com 1/2 system of primary/secondary stats,
// except psi which assumes it's same 3x limit that is applied when using psi gym
void BattleUnit::processExperience(GameState &state)
{
	int secondaryXP = experiencePoints.accuracy + experiencePoints.bravery +
	                  experiencePoints.psi_attack + experiencePoints.psi_energy +
	                  experiencePoints.reactions;
	if (agent->current_stats.accuracy < 100)
	{
		agent->current_stats.accuracy += rollForPrimaryStat(
		    state, experiencePoints.accuracy * agent->type->improvementPercentagePhysical / 100);
	}
	if (agent->current_stats.psi_attack < 100 &&
	    agent->current_stats.psi_attack < agent->initial_stats.psi_attack * 3)
	{
		agent->current_stats.psi_attack += rollForPrimaryStat(
		    state, experiencePoints.psi_attack * agent->type->improvementPercentagePsi / 100);
	}
	if (agent->current_stats.psi_energy < 100 &&
	    agent->current_stats.psi_energy < agent->initial_stats.psi_energy * 3)
	{
		agent->current_stats.psi_energy += rollForPrimaryStat(
		    state, experiencePoints.psi_energy * agent->type->improvementPercentagePsi / 100);
	}
	if (agent->current_stats.reactions < 100)
	{
		if (state.current_battle->mode == Battle::Mode::TurnBased)
		{
			agent->current_stats.reactions +=
			    rollForPrimaryStat(state, experiencePoints.reactions *
			                                  agent->type->improvementPercentagePhysical / 100);
		}
		else
		{
			agent->current_stats.reactions += rollForPrimaryStat(
			    state, std::min(3, secondaryXP * agent->type->improvementPercentagePhysical / 100));
		}
	}
	if (agent->current_stats.bravery < 100)
	{
		agent->current_stats.bravery +=
		    10 * randBoundsExclusive(state.rng, 0, 99) <
		    experiencePoints.bravery * 9 * agent->type->improvementPercentagePhysical / 100;
	}
	// Units with slower improvement rates need to gain more xp to have a chance to improve
	// >= 100% improvement rate need just 1 xp
	// 50% improvement rate needs 2 xp
	// 10% improvement rate needs 10 xp
	// ---
	// Percentile increase also affected (units with 100% improvement gain 10% of their missing
	// stat, units with 50% gain 5% etc)
	if (agent->type->improvementPercentagePhysical > 0 &&
	    secondaryXP > 100 / agent->type->improvementPercentagePhysical)
	{
		if (agent->current_stats.health < 100)
		{
			int healthBoost = randBoundsInclusive(state.rng, 0, 2) +
			                  (100 - agent->current_stats.health) / 10 *
			                      agent->type->improvementPercentagePhysical / 100;
			agent->current_stats.health += healthBoost;
			agent->modified_stats.health += healthBoost;
		}
		if (agent->current_stats.speed < 100)
		{
			agent->current_stats.speed += randBoundsInclusive(state.rng, 0, 2) +
			                              (100 - agent->current_stats.speed) / 10 *
			                                  agent->type->improvementPercentagePhysical / 100;
		}
		if (agent->current_stats.stamina < 2000)
		{
			agent->current_stats.stamina += randBoundsInclusive(state.rng, 0, 2) * 20 +
			                                (2000 - agent->current_stats.stamina) / 10 *
			                                    agent->type->improvementPercentagePhysical / 100;
		}
		if (agent->current_stats.strength < 100)
		{
			agent->current_stats.strength += randBoundsInclusive(state.rng, 0, 2) +
			                                 (100 - agent->current_stats.strength) / 10 *
			                                     agent->type->improvementPercentagePhysical / 100;
		}
	}
	agent->updateModifiedStats();
}

void BattleUnit::executeGroupAIDecision(GameState &state, AIDecision &decision,
                                        std::list<StateRef<BattleUnit>> &units)
{
	if (units.size() == 1)
	{
		units.front()->executeAIDecision(state, decision);
		return;
	}
	if (decision.action)
	{
		for (auto &u : units)
		{
			u->executeAIAction(state, *decision.action);
		}
	}
	if (decision.movement)
	{
		switch (decision.movement->type)
		{
			case AIMovement::Type::Patrol:
				// Stance change
				for (auto &u : units)
				{
					u->setKneelingMode(decision.movement->kneelingMode);
					u->setMovementMode(decision.movement->movementMode);
				}
				state.current_battle->groupMove(state, units, decision.movement->targetLocation);
				break;
			default:
				for (auto &u : units)
				{
					u->executeAIMovement(state, *decision.movement);
				}
				break;
		}
	}
}

void BattleUnit::executeAIDecision(GameState &state, AIDecision &decision)
{
	if (decision.action)
	{
		executeAIAction(state, *decision.action);
	}
	if (decision.movement)
	{
		executeAIMovement(state, *decision.movement);
	}
}

void BattleUnit::executeAIAction(GameState &state, AIAction &action)
{
	if (action.inProgress(*this))
	{
		return;
	}

	// Equip item we're going to use
	if (action.item)
	{
		UnitAIHelper::ensureItemInSlot(state, action.item, EquipmentSlotType::RightHand);
		if (action.type == AIAction::Type::AttackWeaponUnit ||
		    action.type == AIAction::Type::AttackWeaponTile)
		{
			action.item = nullptr;
			action.weaponStatus = WeaponStatus::FiringRightHand;
		}
	}

	// Do it
	switch (action.type)
	{
		case AIAction::Type::AttackWeaponTile:
			startAttacking(state, action.targetLocation, action.weaponStatus);
			break;
		case AIAction::Type::AttackWeaponUnit:
			startAttacking(state, action.targetUnit, action.weaponStatus);
			break;
		case AIAction::Type::AttackGrenade:
			if (action.item->getCanThrow(*this, action.targetUnit->position))
			{
				if (setMission(state, BattleUnitMission::throwItem(*this, action.item,
				                                                   action.targetUnit->position)))
				{
					action.item->prime(action.item->getPayloadType()->trigger_type !=
					                       TriggerType::Boomeroid,
					                   0, 12.0f);
				}
			}
			break;
		case AIAction::Type::AttackPsiMC:
			startAttackPsi(state, action.targetUnit, PsiStatus::Control, action.item->type);
			break;
		case AIAction::Type::AttackPsiStun:
			startAttackPsi(state, action.targetUnit, PsiStatus::Stun, action.item->type);
			break;
		case AIAction::Type::AttackPsiPanic:
			startAttackPsi(state, action.targetUnit, PsiStatus::Panic, action.item->type);
			break;
		case AIAction::Type::AttackBrainsucker:
			useBrainsucker(state);
			break;
		case AIAction::Type::AttackSuicide:
			die(state);
			break;
	}
}

void BattleUnit::executeAIMovement(GameState &state, AIMovement &movement)
{
	if (movement.inProgress(*this))
	{
		return;
	}

	// FIXME: USE teleporter to move?
	// Or maybe this is done in AI?

	// Stance change
	switch (movement.type)
	{
		case AIMovement::Type::Stop:
		case AIMovement::Type::Turn:
			break;
		default:
			setKneelingMode(movement.kneelingMode);
			if (movement_mode != movement.movementMode)
			{
				setMovementMode(movement.movementMode);
			}
			break;
	}

	// Do movement
	switch (movement.type)
	{
		case AIMovement::Type::Stop:
			for (auto &m : missions)
			{
				if (m->type == BattleUnitMission::Type::GotoLocation)
				{
					// Soft cancel
					m->targetLocation = goalPosition;
					m->currentPlannedPath.clear();
				}
			}
			aiList.reportExecuted(movement);
			break;
		case AIMovement::Type::Advance:
		case AIMovement::Type::GetInRange:
		case AIMovement::Type::TakeCover:
		case AIMovement::Type::Patrol:
		case AIMovement::Type::Pursue:
			if (setMission(state, BattleUnitMission::gotoLocation(*this, movement.targetLocation)))
			{
				aiList.reportExecuted(movement);
			}
			break;
		case AIMovement::Type::Retreat:
			if (setMission(state, BattleUnitMission::gotoLocation(*this, movement.targetLocation, 0,
			                                                      true, true, 1, true)))
			{
				aiList.reportExecuted(movement);
			}
			break;
		case AIMovement::Type::Turn:
			if (setMission(state, BattleUnitMission::turn(*this, movement.targetLocation)))
			{
				aiList.reportExecuted(movement);
			}
			break;
		case AIMovement::Type::ChangeStance:
			// Nothing, already applied above
			aiList.reportExecuted(movement);
			break;
	}
}

void BattleUnit::notifyUnderFire(GameState &state, Vec3<int> position, bool visible)
{
	if (!visible)
	{
		// Alert unit under fire if attacker is unseen
		sendAgentEvent(state, GameEventType::AgentUnderFire, true);
	}
	aiList.notifyUnderFire(position);
}

void BattleUnit::notifyHit(Vec3<int> position) { aiList.notifyHit(position); }

void BattleUnit::tryToRiseUp(GameState &state)
{
	// Do not rise up if unit is standing on us
	if (tileObject->getOwningTile()->getUnitIfPresent(true, true, false, tileObject))
		return;

	// Find state we can rise into (with animation)
	auto targetState = BodyState::Standing;
	while (agent->getAnimationPack()->getFrameCountBody(displayedItem, current_body_state,
	                                                    targetState, current_hand_state,
	                                                    current_movement_state, facing) == 0)
	{
		switch (targetState)
		{
			case BodyState::Standing:
				if (agent->isBodyStateAllowed(BodyState::Flying))
				{
					targetState = BodyState::Flying;
					continue;
				}
			// Intentional fall-through
			case BodyState::Flying:
				if (agent->isBodyStateAllowed(BodyState::Kneeling))
				{
					targetState = BodyState::Kneeling;
					continue;
				}
			// Intentional fall-through
			case BodyState::Kneeling:
				if (canProne(position, facing))
				{
					targetState = BodyState::Prone;
					continue;
				}
			// Intentional fall-through
			case BodyState::Prone:
				// If we arrived here then we have no animation for standing up
				targetState = BodyState::Downed;
				break;
			case BodyState::Downed:
			case BodyState::Dead:
			case BodyState::Jumping:
			case BodyState::Throwing:
				LogError("Not possible to reach this?");
				break;
		}
		break;
	}
	// Find state we can rise into (with no animation)
	if (targetState == BodyState::Downed)
	{
		if (agent->isBodyStateAllowed(BodyState::Standing))
		{
			targetState = BodyState::Standing;
		}
		else if (agent->isBodyStateAllowed(BodyState::Flying))
		{
			targetState = BodyState::Flying;
		}
		else if (agent->isBodyStateAllowed(BodyState::Kneeling))
		{
			targetState = BodyState::Kneeling;
		}
		else
		{
			// Prone only unit ignores whether prone is allowed
			targetState = BodyState::Prone;
		}
	}

	missions.clear();
	aiList.reset(state, *this);
	addMission(state, BattleUnitMission::changeStance(*this, targetState));
	state.current_battle->checkMissionEnd(state, false, true);
}

void BattleUnit::dropDown(GameState &state)
{
	state.current_battle->checkMissionEnd(state, false);
	state.current_battle->checkIfBuildingDisabled(state);
	// Reset states, cancel actions
	cloakTicksAccumulated = 0;
	stopAttacking();
	stopAttackPsi(state);
	for (auto &a : psiAttackers)
	{
		auto attacker = StateRef<BattleUnit>(&state, a.first);
		if (attacker->psiStatus != PsiStatus::Stun)
		{
			attacker->stopAttackPsi(state);
		}
	}
	aiList.reset(state, *this);
	resetGoal();
	setHandState(HandState::AtEase);
	setBodyState(state, target_body_state);
	startFalling(state);
	brainSucker.clear();
	fireDebuffTicksRemaining = 0;
	enzymeDebuffIntensity = 0;
	moraleState = MoraleState::Normal;
	BodyState targetState = isDead() ? BodyState::Dead : BodyState::Downed;
	// Check if we can drop from current state
	// Adjust current state so that we can then drop down
	BodyState proposedBodyState = current_body_state;
	while (agent->getAnimationPack()->getFrameCountBody(displayedItem, proposedBodyState,
	                                                    targetState, current_hand_state,
	                                                    current_movement_state, facing) == 0)
	{
		switch (proposedBodyState)
		{
			case BodyState::Jumping:
			case BodyState::Throwing:
			case BodyState::Flying:
				if (agent->isBodyStateAllowed(BodyState::Standing))
				{
					proposedBodyState = BodyState::Standing;
					continue;
				}
			// Intentional fall-through
			case BodyState::Standing:
				if (agent->isBodyStateAllowed(BodyState::Kneeling))
				{
					proposedBodyState = BodyState::Kneeling;
					continue;
				}
			// Intentional fall-through
			case BodyState::Kneeling:
				proposedBodyState = BodyState::Prone;
				continue;
			// Unit has no downed animation, abort
			case BodyState::Prone:
			case BodyState::Downed:
				proposedBodyState = BodyState::Downed;
				break;
			case BodyState::Dead:
				LogError("Impossible");
				break;
		}
		break;
	}
	setBodyState(state, proposedBodyState);
	cancelMissions(state, true);
	// Drop all gear
	if (agent->type->inventory)
	{
		auto it = agent->equipment.begin();
		while (it != agent->equipment.end())
		{
			auto e = *it++;
			if (e->type->type != AEquipmentType::Type::Armor)
			{
				addMission(state, BattleUnitMission::dropItem(*this, e));
			}
		}
	}
	// Drop down
	addMission(state, BattleUnitMission::changeStance(*this, targetState));
	// Drop all armor after going down
	if (agent->type->inventory)
	{
		for (auto e : agent->equipment)
		{
			if (e->type->type == AEquipmentType::Type::Armor)
			{
				addMission(state, BattleUnitMission::dropItem(*this, e), true);
			}
		}
	}

	this->markUnVisible(state);
}

void BattleUnit::markUnVisible(GameState &state)
{ // Remove from list of visible units
	StateRef<BattleUnit> srThis = {&state, id};
	for (auto &units : state.current_battle->visibleUnits)
	{
		if (units.first != owner)
		{
			units.second.erase(srThis);
		}
	}
	for (auto &units : state.current_battle->visibleEnemies)
	{
		if (units.first != owner)
		{
			units.second.erase(srThis);
		}
	}
	for (auto &unit : state.current_battle->units)
	{
		if (unit.second->owner != owner)
		{
			unit.second->visibleUnits.erase(srThis);
			unit.second->visibleEnemies.erase(srThis);
		}
	}
}

void BattleUnit::retreat(GameState &state)
{
	this->markUnVisible(state);
	if (shadowObject)
	{
		shadowObject->removeFromMap();
	}
	tileObject->removeFromMap();
	stopAttackPsi(state);
	retreated = true;
	removeFromSquad(*state.current_battle);
	state.current_battle->refreshLeadershipBonus(agent->owner);
	sendAgentEvent(state, GameEventType::AgentLeftCombat, true);
	state.current_battle->checkMissionEnd(state, true);
}

bool BattleUnit::useSpawner(GameState &state, const AEquipmentType &item)
{
	std::list<Vec3<int>> posToCheck;
	Vec3<int> curPos = position;
	posToCheck.push_back(curPos + Vec3<int>{1, 0, 0});
	posToCheck.push_back(curPos + Vec3<int>{0, 1, 0});
	posToCheck.push_back(curPos + Vec3<int>{-1, 0, 0});
	posToCheck.push_back(curPos + Vec3<int>{0, -1, 0});
	posToCheck.push_back(curPos + Vec3<int>{1, 1, 0});
	posToCheck.push_back(curPos + Vec3<int>{1, -1, 0});
	posToCheck.push_back(curPos + Vec3<int>{-1, 1, 0});
	posToCheck.push_back(curPos + Vec3<int>{-1, -1, 0});
	posToCheck.push_back(curPos + Vec3<int>{2, 0, 0});
	posToCheck.push_back(curPos + Vec3<int>{0, 2, 0});
	posToCheck.push_back(curPos + Vec3<int>{-2, 0, 0});
	posToCheck.push_back(curPos + Vec3<int>{0, -2, 0});
	std::list<Vec3<int>> posToSpawn;
	posToSpawn.push_back(curPos);
	int numToSpawn = -1;
	for (const auto &entry : item.spawnList)
	{
		numToSpawn += entry.second;
	}
	auto &map = tileObject->map;
	auto helper = BattleUnitTileHelper(map, *this);
	while (numToSpawn > 0 && !posToCheck.empty())
	{
		auto pos = posToCheck.front();
		posToCheck.pop_front();
		if (!map.tileIsValid(pos))
		{
			continue;
		}
		if (state.current_battle->findShortestPath(curPos, pos, helper, false, false, true, 0, true)
		        .back() == pos)
		{
			numToSpawn--;
			posToSpawn.push_back(pos);
		}
	}
	auto aliens = state.getAliens();
	for (const auto &entry : item.spawnList)
	{
		for (int i = 0; i < entry.second; i++)
		{
			if (posToSpawn.empty())
			{
				break;
			}
			auto pos = posToSpawn.front();
			posToSpawn.pop_front();
			state.current_battle->spawnUnit(state, aliens, entry.first,
			                                {pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.1f}, facing,
			                                BodyState::Standing);
		}
	}
	return true;
}

void BattleUnit::die(GameState &state, StateRef<BattleUnit> attacker, bool violently)
{
	auto attackerOrg = attacker ? attacker->agent->owner : nullptr;
	auto ourOrg = agent->owner;
	// Violent deaths (spawn stuff, blow up)
	if (violently)
	{
		for (auto &e : agent->equipment)
		{
			switch (e->type->type)
			{
				// Blow up
				case AEquipmentType::Type::Popper:
					state.current_battle->addExplosion(state, position,
					                                   e->type->damage_type->explosionDoodad,
					                                   e->type->damage_type, e->type->damage,
					                                   e->type->explosion_depletion_rate, owner);
					break;
				case AEquipmentType::Type::Spawner:
				{
					useSpawner(state, *e->type);
					break;
				}
				default:
					break;
			}
		}
	}
	// Clear focus
	for (auto &u : focusedByUnits)
	{
		u->focusUnit.clear();
	}
	focusedByUnits.clear();
	// Emit sound
	if (agent->type->dieSfx.find(agent->gender) != agent->type->dieSfx.end() &&
	    !agent->type->dieSfx.at(agent->gender).empty())
	{
		fw().soundBackend->playSample(pickRandom(state.rng, agent->type->dieSfx.at(agent->gender)),
		                              position);
	}
	// Morale and score
	auto player = state.getPlayer();
	// Find next highest ranking officer
	state.current_battle->refreshLeadershipBonus(ourOrg);
	// Penalty for teamkills or bonus for eliminating a hostile
	if (attacker && attackerOrg == ourOrg)
	{
		// Teamkill penalty morale
		attacker->agent->modified_stats.loseMorale(20);
		attacker->combatRating -= agent->type->score;
		// Teamkill penalty score
		if (ourOrg == player)
		{
			state.current_battle->score.friendlyFire -= 10;
		}
	}
	else if (attacker && attackerOrg->isRelatedTo(ourOrg) == Organisation::Relation::Hostile)
	{
		// Bonus for killing a hostile
		attacker->agent->modified_stats.gainMorale(
		    20 * state.current_battle->leadershipBonus[attackerOrg]);
		for (auto &u : state.current_battle->units)
		{
			if (u.first != attacker.id || !u.second->isConscious() ||
			    u.second->agent->owner != attackerOrg)
			{
				continue;
			}
			// Agents from attacker team gain morale
			u.second->agent->modified_stats.gainMorale(
			    10 * state.current_battle->leadershipBonus[attackerOrg]);
		}
	}
	// Adjust relationships
	if (attacker)
	{
		// If we're hostile to attacker - lose 5 points
		if (ourOrg->isRelatedTo(attackerOrg) == Organisation::Relation::Hostile)
		{
			ourOrg->adjustRelationTo(state, attackerOrg, -5.0f);
		}
		// If we're not hostile to attacker - lose 30 points
		else
		{
			ourOrg->adjustRelationTo(state, attackerOrg, -30.0f);
		}
		// Our allies lose 5 points, enemies gain 2 points
		for (auto &org : state.organisations)
		{
			if (org.first != attackerOrg.id && org.first != state.getCivilian().id)
			{
				if (org.second->isRelatedTo(ourOrg) == Organisation::Relation::Hostile)
				{
					org.second->adjustRelationTo(state, attackerOrg, 2.0f);
				}
				else if (org.second->isRelatedTo(ourOrg) == Organisation::Relation::Allied)
				{
					org.second->adjustRelationTo(state, attackerOrg, -5.0f);
				}
			}
		}
	}
	// Score for death/kill
	if (ourOrg == player)
	{
		state.current_battle->score.casualtyPenalty -= agent->type->score;
	}
	else if (attacker && attackerOrg == player &&
	         player->isRelatedTo(ourOrg) == Organisation::Relation::Hostile)
	{
		attacker->combatRating += agent->type->score;
		state.current_battle->score.combatRating += agent->type->score;
	}
	// Penalty for unit in squad dying
	int moraleLossPenalty = 0;
	switch (agent->rank)
	{
		case Rank::Rookie:
		case Rank::Squaddie:
			moraleLossPenalty = 100;
			break;
		case Rank::SquadLeader:
			moraleLossPenalty = 110;
			break;
		case Rank::Sergeant:
			moraleLossPenalty = 120;
			break;
		case Rank::Captain:
			moraleLossPenalty = 130;
			break;
		case Rank::Colonel:
			moraleLossPenalty = 150;
			break;
		case Rank::Commander:
			moraleLossPenalty = 175;
			break;
	}
	// Surviving units lose morale
	for (auto &u : state.current_battle->units)
	{
		if (u.second->agent->owner != ourOrg)
		{
			continue;
		}
		u.second->agent->modified_stats.loseMorale(
		    (110 - u.second->agent->modified_stats.bravery) /
		    (100 + state.current_battle->leadershipBonus[ourOrg]) * moraleLossPenalty / 500);
	}
	// Events
	if (owner == state.current_battle->currentPlayer)
	{
		sendAgentEvent(state, GameEventType::AgentDiedBattle);
	}
	else if (state.current_battle->currentPlayer->isRelatedTo(owner) ==
	         Organisation::Relation::Hostile)
	{
		sendAgentEvent(state, GameEventType::HostileDied);
	}
	// Leave squad
	removeFromSquad(*state.current_battle);
	// Agent also dies
	agent->die(state);
	// Animate body
	dropDown(state);
}

void BattleUnit::fallUnconscious(GameState &state, StateRef<BattleUnit> attacker)
{
	// Adjust relationships
	if (attacker)
	{
		if (config().getBool("OpenApoc.Mod.StunHostileAction"))
		{
			auto attackerOrg = attacker->agent->owner;
			auto ourOrg = agent->owner;

			// If we're hostile to attacker - lose 3 points
			if (ourOrg->isRelatedTo(attackerOrg) == Organisation::Relation::Hostile)
			{
				ourOrg->adjustRelationTo(state, attackerOrg, -3.0f);
			}
			// If we're not hostile to attacker - lose 20 points
			else
			{
				ourOrg->adjustRelationTo(state, attackerOrg, -20.0f);
			}
			// Our allies lose 3 points, enemies gain 1.5 points
			for (auto &org : state.organisations)
			{
				if (org.first != attackerOrg.id && org.first != state.getCivilian().id)
				{
					if (org.second->isRelatedTo(ourOrg) == Organisation::Relation::Hostile)
					{
						org.second->adjustRelationTo(state, attackerOrg, 1.5f);
					}
					else if (org.second->isRelatedTo(ourOrg) == Organisation::Relation::Allied)
					{
						org.second->adjustRelationTo(state, attackerOrg, -3.0f);
					}
				}
			}
		}
	}
	sendAgentEvent(state, GameEventType::AgentUnconscious, true);
	dropDown(state);
}

void BattleUnit::beginBodyStateChange(GameState &state, BodyState bodyState)
{

	// This is ONLY called from BattleUnit::updateBody, which already ensures that:
	// - bodyState is legal for this unit
	// - there is no current body change going on
	// Therefore, we're not checking here for those conditions

	// Already in target body state -> Exit
	if (target_body_state == bodyState)
	{
		return;
	}

	// Cease hand animation immediately
	if (hand_animation_ticks_remaining != 0)
		setHandState(target_hand_state);

	// Find which animation is possible
	int frameCount = agent->getAnimationPack()->getFrameCountBody(displayedItem, current_body_state,
	                                                              bodyState, current_hand_state,
	                                                              current_movement_state, facing);
	// No such animation -> Try without movement
	if (frameCount == 0 && current_movement_state != MovementState::None)
	{
		frameCount = agent->getAnimationPack()->getFrameCountBody(displayedItem, current_body_state,
		                                                          bodyState, current_hand_state,
		                                                          MovementState::None, facing);
		if (frameCount != 0)
		{
			setMovementState(MovementState::None);
		}
	}
	// No such animation -> Try without aiming
	if (frameCount == 0 && current_hand_state != HandState::AtEase)
	{
		frameCount = agent->getAnimationPack()->getFrameCountBody(displayedItem, current_body_state,
		                                                          bodyState, HandState::AtEase,
		                                                          current_movement_state, facing);
		if (frameCount != 0)
		{
			setHandState(HandState::AtEase);
		}
	}
	// No such animation -> Try without both
	if (frameCount == 0 && current_movement_state != MovementState::None &&
	    current_hand_state != HandState::AtEase)
	{
		frameCount = agent->getAnimationPack()->getFrameCountBody(displayedItem, current_body_state,
		                                                          bodyState, HandState::AtEase,
		                                                          MovementState::None, facing);
		if (frameCount != 0)
		{
			setMovementState(MovementState::None);
			setHandState(HandState::AtEase);
		}
	}
	int ticks = frameCount * TICKS_PER_FRAME_UNIT;
	if (ticks > 0)
	{
		target_body_state = bodyState;
		body_animation_ticks_total = ticks;
		body_animation_ticks_remaining = ticks;
		// Updates bounds etc.
		if (tileObject)
		{
			setPosition(state, position);
		}
	}
	else
	{
		setBodyState(state, bodyState);
	}
}

bool BattleUnit::useBrainsucker(GameState &state)
{
	StateRef<DamageType> brainsucker = {&state, "DAMAGETYPE_BRAINSUCKER"};
	Vec3<int> targetPos = position;
	// Fill target list
	std::list<Vec3<int>> targetList;
	targetList.push_back(targetPos + Vec3<int>(facing.x, facing.y, 0));
	targetList.push_back(targetPos + Vec3<int>(facing.x, facing.y, 1));
	targetList.push_back(targetPos + Vec3<int>(facing.x, facing.y, -1));
	if (facing.x == 0)
	{
		targetList.push_back(targetPos + Vec3<int>(1, facing.y, 0));
		targetList.push_back(targetPos + Vec3<int>(1, facing.y, 1));
		targetList.push_back(targetPos + Vec3<int>(1, facing.y, -1));
		targetList.push_back(targetPos + Vec3<int>(-1, facing.y, 0));
		targetList.push_back(targetPos + Vec3<int>(-1, facing.y, 1));
		targetList.push_back(targetPos + Vec3<int>(-1, facing.y, -1));
	}
	else if (facing.y == 0)
	{
		targetList.push_back(targetPos + Vec3<int>(facing.x, 1, 0));
		targetList.push_back(targetPos + Vec3<int>(facing.x, 1, 1));
		targetList.push_back(targetPos + Vec3<int>(facing.x, 1, -1));
		targetList.push_back(targetPos + Vec3<int>(facing.x, -1, 0));
		targetList.push_back(targetPos + Vec3<int>(facing.x, -1, 1));
		targetList.push_back(targetPos + Vec3<int>(facing.x, -1, -1));
	}
	else
	{
		targetList.push_back(targetPos + Vec3<int>(facing.x, 0, 0));
		targetList.push_back(targetPos + Vec3<int>(facing.x, 0, 1));
		targetList.push_back(targetPos + Vec3<int>(facing.x, 0, -1));
		targetList.push_back(targetPos + Vec3<int>(0, facing.y, 0));
		targetList.push_back(targetPos + Vec3<int>(0, facing.y, 1));
		targetList.push_back(targetPos + Vec3<int>(0, facing.y, -1));
		targetList.push_back(targetPos + Vec3<int>(facing.x, -facing.y, 0));
		targetList.push_back(targetPos + Vec3<int>(facing.x, -facing.y, 1));
		targetList.push_back(targetPos + Vec3<int>(facing.x, -facing.y, -1));
		targetList.push_back(targetPos + Vec3<int>(-facing.x, facing.y, 0));
		targetList.push_back(targetPos + Vec3<int>(-facing.x, facing.y, 1));
		targetList.push_back(targetPos + Vec3<int>(-facing.x, facing.y, -1));
	}
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			if (x == 0 && y == 0)
			{
				continue;
			}
			if ((facing.x != 0 && x == facing.x) || (facing.y != 0 && y == facing.y))
			{
				continue;
			}
			targetList.push_back(targetPos + Vec3<int>(x, y, 0));
			targetList.push_back(targetPos + Vec3<int>(x, y, 1));
			targetList.push_back(targetPos + Vec3<int>(x, y, -1));
		}
	}
	auto &map = tileObject->map;
	auto helper = BattleUnitTileHelper(map, *this);
	auto ourTile = tileObject->getOwningTile();
	for (auto &pos : targetList)
	{
		if (!map.tileIsValid(pos))
		{
			continue;
		}
		auto targetTile = map.getTile(pos);
		if (!helper.canEnterTile(ourTile, targetTile, true, true, true))
		{
			continue;
		}
		if (targetTile->getUnitIfPresent(true))
		{
			auto target = targetTile->getUnitIfPresent(true)->getUnit();
			if (brainsucker->dealDamage(100, target->agent->type->damage_modifier) != 0)
			{
				// Account for target movement
				auto targetPosAdjusted = target->getMuzzleLocation();
				Vec3<float> targetVelocity =
				    target->getVelocity() / (float)TICK_SCALE / VELOCITY_SCALE_BATTLE;
				// Scale to about 0.8 seconds, that's the amount
				// it normally takes sucker to arrive
				targetVelocity *= 0.8f * (float)TICKS_PER_SECOND;
				// We don't care about z differences
				targetVelocity.z = 0.0f;
				targetPosAdjusted += targetVelocity;

				// Go for the head!
				setMission(state, BattleUnitMission::jump(*this, targetPosAdjusted,
				                                          BodyState::Jumping, false));
				return true;
			}
		}
	}
	return false;
}

bool BattleUnit::useItem(GameState &state, sp<AEquipment> item)
{
	if (item->ownerAgent != agent || (item->equippedSlotType != EquipmentSlotType::RightHand &&
	                                  item->equippedSlotType != EquipmentSlotType::LeftHand))
	{
		LogError("Unit %s attempting to use item that is not in hand or does not belong to us?",
		         id);
		return false;
	}
	switch (item->type->type)
	{
		// Items that cannot be used this way
		case AEquipmentType::Type::Weapon:
		case AEquipmentType::Type::Grenade:
		case AEquipmentType::Type::MindBender:
		case AEquipmentType::Type::Teleporter:
			return false;
		// Items that do nothing
		case AEquipmentType::Type::AlienDetector:
		case AEquipmentType::Type::Ammo:
		case AEquipmentType::Type::Armor:
		case AEquipmentType::Type::CloakingField:
		case AEquipmentType::Type::DimensionForceField:
		case AEquipmentType::Type::DisruptorShield:
		case AEquipmentType::Type::Loot:
		case AEquipmentType::Type::MindShield:
		case AEquipmentType::Type::MultiTracker:
		case AEquipmentType::Type::StructureProbe:
		case AEquipmentType::Type::VortexAnalyzer:
			return false;
		case AEquipmentType::Type::MotionScanner:
			if (item->inUse)
			{
				state.current_battle->removeScanner(state, *item);
			}
			else
			{
				if (state.current_battle->mode == Battle::Mode::TurnBased)
				{
					// 10% of max TUs
					if (!spendTU(state, getMotionScannerCost()))
					{
						LogWarning("Notify unsufficient TU for motion scanner");
						return false;
					}
				}
				else
				{
					state.current_battle->addScanner(state, *item);
				}
			}
			item->inUse = !item->inUse;
			return true;
		case AEquipmentType::Type::MediKit:
			// Initial use of medikit just brings up interface, action and TU spent happens
			// when individual body part is clicked
			item->inUse = !item->inUse;
			return true;
		case AEquipmentType::Type::Brainsucker:
			useBrainsucker(state);
			break;
		case AEquipmentType::Type::Popper:
		case AEquipmentType::Type::Spawner:
			// Just suicide, explosion/spawn will happen automatically
			die(state);
			return true;
	}

	return false;
}

bool BattleUnit::useMedikit(GameState &state, BodyPart part)
{
	if (fatalWounds[part] == 0)
	{
		return false;
	}

	if (state.current_battle->mode == Battle::Mode::TurnBased)
	{
		// 37.5% of max TUs
		if (!spendTU(state, getMedikitCost()))
		{
			LogWarning("Notify unsufficient TU for medikit");
			return false;
		}
		fatalWounds[part]--;
		agent->modified_stats.health += 5;
	}
	else
	{
		isHealing = true;
		healingBodyPart = part;
	}
	return true;
}

unsigned int BattleUnit::getBodyAnimationFrame() const
{
	return body_animation_ticks_remaining > 0
	           ? (body_animation_ticks_remaining + TICKS_PER_FRAME_UNIT - 1) / TICKS_PER_FRAME_UNIT
	           : body_animation_ticks_static / TICKS_PER_FRAME_UNIT;
}

void BattleUnit::setBodyState(GameState &state, BodyState bodyState)
{
	if (!agent->isBodyStateAllowed(bodyState))
	{
		LogError("SetBodyState called on %s (%s) (%s) with bodyState %d", id, agent->name,
		         agent->type->id, (int)bodyState);
		return;
	}
	bool roseUp = current_body_state == BodyState::Downed;
	current_body_state = bodyState;
	target_body_state = bodyState;
	body_animation_ticks_remaining = 0;
	body_animation_ticks_total = 1;
	// Update things that require tileObject (or stuff loaded after battle::init)
	if (tileObject)
	{
		// Ensure we have frames in this state
		auto animationPack = agent->getAnimationPack();
		if (animationPack->getFrameCountBody(displayedItem, current_body_state, target_body_state,
		                                     current_hand_state, current_movement_state,
		                                     facing) == 0)
		{
			// No animation for target state -> Try without movement
			if (current_movement_state != MovementState::None &&
			    animationPack->getFrameCountBody(displayedItem, current_body_state,
			                                     target_body_state, current_hand_state,
			                                     MovementState::None, facing) != 0)
			{
				setMovementState(MovementState::None);
			}
			// No animation for target state -> Try without aiming
			else if (current_hand_state != HandState::AtEase &&
			         animationPack->getFrameCountBody(displayedItem, current_body_state,
			                                          target_body_state, HandState::AtEase,
			                                          current_movement_state, facing) != 0)
			{
				setHandState(HandState::AtEase);
			}
			// We know for sure this body state is allowed,
			// and we know for sure allowed body state has frames w/o aiming and moving
			else
			{
				setMovementState(MovementState::None);
				setHandState(HandState::AtEase);
			}
		}
		// Updates bounds etc.
		setPosition(state, position);
		// Update vision since our head position may have changed
		refreshUnitVision(state);
		// If rose up - update vision for units that see this
		if (roseUp)
		{
			refreshUnitVisibilityAndVision(state);
		}
	}
}

unsigned int BattleUnit::getHandAnimationFrame() const
{
	return ((firing_animation_ticks_remaining > 0 ? firing_animation_ticks_remaining
	                                              : hand_animation_ticks_remaining) +
	        TICKS_PER_FRAME_UNIT - 1) /
	       TICKS_PER_FRAME_UNIT;
}

bool BattleUnit::canHandStateChange(HandState state)
{
	return firing_animation_ticks_remaining == 0 && hand_animation_ticks_remaining == 0 &&
	       body_animation_ticks_remaining == 0 && target_hand_state != state &&
	       (state == HandState::AtEase ||
	        (agent->isFireDuringMovementStateAllowed(current_movement_state) &&
	         current_body_state != BodyState::Throwing &&
	         current_body_state != BodyState::Jumping &&
	         (current_movement_state == MovementState::None ||
	          current_body_state != BodyState::Prone)));
}

void BattleUnit::beginHandStateChange(HandState state)
{
	// This is ONLY called after confirming with canHandStateChange that change is valid
	// Therefore, we're not checking for conditions

	if (target_hand_state == state)
	{
		return;
	}

	int frameCount = agent->getAnimationPack()->getFrameCountHands(
	    displayedItem, current_body_state, current_hand_state, state, current_movement_state,
	    facing);
	int ticks = frameCount * TICKS_PER_FRAME_UNIT;

	if (ticks > 0 && target_hand_state != state)
	{
		target_hand_state = state;
		hand_animation_ticks_remaining = ticks;
	}
	else
	{
		setHandState(state);
	}
	residual_aiming_ticks_remaining = 0;
}

void BattleUnit::setHandState(HandState state)
{
	current_hand_state = state;
	target_hand_state = state;
	hand_animation_ticks_remaining = 0;
	firing_animation_ticks_remaining =
	    state != HandState::Firing
	        ? 0
	        : agent->getAnimationPack()->getFrameCountFiring(displayedItem, current_body_state,
	                                                         current_movement_state, facing) *
	              TICKS_PER_FRAME_UNIT;
	residual_aiming_ticks_remaining = state == HandState::Aiming ? TICKS_PER_SECOND / 3 : 0;
}

void BattleUnit::beginTurning(GameState &, Vec2<int> newFacing)
{
	goalFacing = newFacing;
	turning_animation_ticks_remaining = TICKS_PER_FRAME_UNIT;
}

void BattleUnit::setFacing(GameState &state, Vec2<int> newFacing)
{
	facing = newFacing;
	goalFacing = newFacing;
	turning_animation_ticks_remaining = 0;
	refreshUnitVision(state);
}

void BattleUnit::setMovementState(MovementState state)
{
	if (!agent->isMovementStateAllowed(state))
	{
		LogError("WTF? Where the hell you're going?");
		return;
	}

	if (current_movement_state == state)
	{
		return;
	}

	switch (state)
	{
		case MovementState::None:
			movement_ticks_passed = 0;
			movement_sounds_played = 0;
			ticksUntillNextTargetCheck = 0;
			break;
		case MovementState::Running:
			if (current_hand_state != HandState::AtEase || target_hand_state != HandState::AtEase)
			{
				setHandState(HandState::AtEase);
			}
			break;
		case MovementState::Strafing:
			// Nothing TBD
			break;
		default:
			break;
	}

	// Ensure we have frames in this state
	if (agent->getAnimationPack()->getFrameCountBody(displayedItem, current_body_state,
	                                                 target_body_state, current_hand_state, state,
	                                                 facing) == 0)
	{
		// No animation for target state -> Try without aiming
		if (current_hand_state != HandState::AtEase &&
		    agent->getAnimationPack()->getFrameCountBody(displayedItem, current_body_state,
		                                                 target_body_state, HandState::AtEase,
		                                                 state, facing) != 0)
		{
			setHandState(HandState::AtEase);
		}
		else
		{
			LogError("setMovementState %d incompatible with current body states %d to %d",
			         (int)state, (int)current_body_state, (int)target_body_state);
			return;
		}
	}

	current_movement_state = state;
}

void BattleUnit::setMovementMode(MovementMode mode)
{
	switch (mode)
	{
		case MovementMode::Prone:
			if (agent->isBodyStateAllowed(BodyState::Prone))
			{
				movement_mode = MovementMode::Prone;
			}
			break;
		case MovementMode::Walking:
			if (agent->isBodyStateAllowed(BodyState::Standing) ||
			    agent->isBodyStateAllowed(BodyState::Flying))
			{
				movement_mode = MovementMode::Walking;
			}
			break;
		case MovementMode::Running:
			if (agent->isMovementStateAllowed(MovementState::Running))
			{
				movement_mode = MovementMode::Running;
			}
			break;
	}
}

void BattleUnit::setKneelingMode(KneelingMode mode) { kneeling_mode = mode; }

void BattleUnit::setWeaponAimingMode(WeaponAimingMode mode) { fire_aiming_mode = mode; }

void BattleUnit::setFirePermissionMode(FirePermissionMode mode) { fire_permission_mode = mode; }

void BattleUnit::setBehaviorMode(BehaviorMode mode) { behavior_mode = mode; }

unsigned int BattleUnit::getWalkSoundIndex()
{
	if (current_movement_state == MovementState::Running)
	{
		return ((movement_sounds_played + UNITS_TRAVELLED_PER_SOUND_RUNNING_DIVISOR - 1) /
		        UNITS_TRAVELLED_PER_SOUND_RUNNING_DIVISOR) %
		       2;
	}
	else
	{
		return movement_sounds_played % 2;
	}
}

void BattleUnit::playWalkSound(GameState &state)
{
	sp<Sample> walkSfx = nullptr;
	if (agent->type->walkSfx.size() > 0)
	{
		walkSfx = agent->type->walkSfx[getWalkSoundIndex() % agent->type->walkSfx.size()];
	}
	else
	{
		auto t = tileObject->getOwningTile();
		if (t->walkSfx && t->walkSfx->size() > 0)
		{
			walkSfx = t->walkSfx->at(getWalkSoundIndex() % t->walkSfx->size());
		}
	}
	if (walkSfx)
	{
		playDistantSound(state, walkSfx);
	}
}

void BattleUnit::playDistantSound(GameState &state, sp<Sample> sfx, float gainMult)
{
	float distance = FLT_MAX;
	if (owner == state.current_battle->currentPlayer)
	{
		distance = 0.0f;
	}
	else
	{
		for (auto &u : state.current_battle->units)
		{
			if (u.second->owner != state.current_battle->currentPlayer || !u.second->isConscious())
			{
				continue;
			}
			distance = std::min(distance, glm::distance(u.second->position, position));
		}
	}
	if (distance < MAX_HEARING_DISTANCE)
	{

		fw().soundBackend->playSample(sfx, getPosition(),
		                              gainMult * (MAX_HEARING_DISTANCE - distance) /
		                                  MAX_HEARING_DISTANCE);
	}
}

void BattleUnit::initCryTimer(GameState &state)
{
	// FIXME: Implement proper cry timers
	ticksUntillNextCry =
	    2 * TICKS_PER_SECOND + randBoundsInclusive(state.rng, 0, 16 * (int)TICKS_PER_SECOND);
}

void BattleUnit::resetCryTimer(GameState &state)
{
	// FIXME: Implement proper cry timers
	ticksUntillNextCry =
	    8 * TICKS_PER_SECOND + randBoundsInclusive(state.rng, 0, 16 * (int)TICKS_PER_SECOND);
}

bool BattleUnit::getNewGoal(GameState &state)
{
	atGoal = true;
	launched = false;

	bool popped = false;
	bool acquired = false;
	popped = popFinishedMissions(state);
	if (retreated)
	{
		return false;
	}
	do
	{
		// Try to get new destination
		acquired = getNextDestination(state, goalPosition);
		// Pop finished missions if present
		popped = popFinishedMissions(state);
		if (retreated)
		{
			return false;
		}
	} while (popped && !acquired);
	if (acquired)
	{
		atGoal = false;
		startMoving(state);
	}
	else if (!hasMovementQueued())
	{
		setMovementState(MovementState::None);
	}
	return acquired;
}

Vec3<float> BattleUnit::getMuzzleLocation() const
{
	return position +
	       Vec3<float>{0.0f, 0.0f,
	                   ((float)agent->type->bodyType->muzzleZPosition.at(current_body_state) *
	                        (body_animation_ticks_remaining) +
	                    (float)agent->type->bodyType->muzzleZPosition.at(target_body_state) *
	                        (body_animation_ticks_total - body_animation_ticks_remaining)) /
	                       (float)body_animation_ticks_total / 40.0f};
}

Vec3<float> BattleUnit::getEyeLocation() const
{
	return Vec3<float>{(int)position.x + 0.5f, (int)position.y + 0.5f,
	                   (int)position.z +
	                       ((float)agent->type->bodyType->muzzleZPosition.at(current_body_state)) /
	                           40.0f};
}

Vec3<float> BattleUnit::getThrownItemLocation() const
{
	return position +
	       Vec3<float>{0.0f, 0.0f,
	                   ((float)agent->type->bodyType->height.at(BodyState::Throwing) - 4.0f) /
	                       2.0f / 40.0f};
}

unsigned int BattleUnit::getDistanceTravelled() const
{
	return movement_ticks_passed / TICKS_PER_UNIT_TRAVELLED_BATTLEUNIT;
}

bool BattleUnit::shouldPlaySoundNow()
{
	if (current_body_state == BodyState::Jumping && target_body_state == BodyState::Jumping)
	{
		return false;
	}
	bool play = false;
	unsigned int sounds_to_play = getDistanceTravelled() / UNITS_TRAVELLED_PER_SOUND;
	if (sounds_to_play != movement_sounds_played)
	{
		unsigned int divisor = (current_movement_state == MovementState::Running)
		                           ? UNITS_TRAVELLED_PER_SOUND_RUNNING_DIVISOR
		                           : 1;
		play = ((sounds_to_play + divisor - 1) % divisor) == 0;
		movement_sounds_played = sounds_to_play;
	}
	return play;
}

bool BattleUnit::popFinishedMissions(GameState &state)
{
	bool popped = false;
	while (missions.size() > 0 && missions.front()->isFinished(state, *this))
	{
		LogWarning("Unit %s mission \"%s\" finished", id, missions.front()->getName());
		missions.pop_front();
		popped = true;
		// We may have retreated as a result of finished mission
		if (retreated)
		{
			return popped;
		}

		if (!missions.empty())
		{
			missions.front()->start(state, *this);
			continue;
		}
		else
		{
			LogWarning("No next unit mission, going idle");
			break;
		}
	}
	return popped;
}

bool BattleUnit::hasMovementQueued()
{
	for (auto &m : missions)
	{
		if (m->type == BattleUnitMission::Type::GotoLocation ||
		    m->type == BattleUnitMission::Type::Brainsuck)
		{
			return true;
		}
	}
	return false;
}

bool BattleUnit::getNextDestination(GameState &state, Vec3<float> &dest)
{
	if (missions.empty())
	{
		return false;
	}
	return missions.front()->getNextDestination(state, *this, dest);
}

bool BattleUnit::getNextFacing(GameState &state, Vec2<int> &dest)
{
	if (missions.empty())
	{
		return false;
	}
	return missions.front()->getNextFacing(state, *this, dest);
}

bool BattleUnit::getNextBodyState(GameState &state, BodyState &dest)
{
	if (missions.empty())
	{
		return false;
	}
	return missions.front()->getNextBodyState(state, *this, dest);
}

bool BattleUnit::addMission(GameState &state, BattleUnitMission::Type type)
{
	switch (type)
	{
		case BattleUnitMission::Type::RestartNextMission:
			return addMission(state, BattleUnitMission::restartNextMission(*this));
		case BattleUnitMission::Type::ReachGoal:
			return addMission(state, BattleUnitMission::reachGoal(*this));
		case BattleUnitMission::Type::AcquireTU:
			return addMission(state, BattleUnitMission::acquireTU(*this));
		case BattleUnitMission::Type::DropItem:
		case BattleUnitMission::Type::ThrowItem:
		case BattleUnitMission::Type::Snooze:
		case BattleUnitMission::Type::ChangeBodyState:
		case BattleUnitMission::Type::Turn:
		case BattleUnitMission::Type::Brainsuck:
		case BattleUnitMission::Type::Jump:
		case BattleUnitMission::Type::GotoLocation:
		case BattleUnitMission::Type::Teleport:
			LogError("Cannot add mission by type if it requires parameters");
			break;
	}
	return false;
}

bool BattleUnit::cancelMissions(GameState &state, bool forced)
{
	if (forced)
	{
		// Drop gear used by missions, remove headcrab
		std::list<sp<AEquipment>> equipToDrop;
		for (auto &m : missions)
		{
			if (m->item && m->item->ownerAgent)
			{
				equipToDrop.push_back(m->item);
			}
			if (m->targetUnit && m->type == BattleUnitMission::Type::Brainsuck)
			{
				m->targetUnit->brainSucker.clear();
			}
		}
		missions.clear();
		for (auto &e : equipToDrop)
		{
			addMission(state, BattleUnitMission::dropItem(*this, e));
		}
	}
	else
	{
		popFinishedMissions(state);
		if (retreated)
		{
			return false;
		}
		if (missions.empty())
		{
			return true;
		}

		// Figure out if we can cancel the mission in front
		bool letFinish = false;
		int facingDelta = 0;
		switch (missions.front()->type)
		{
			// Missions that cannot be cancelled
			case BattleUnitMission::Type::ThrowItem:
			case BattleUnitMission::Type::Brainsuck:
				return false;
			// Jump can only be cancelled if we have not jumped yet
			case BattleUnitMission::Type::Jump:
				return !missions.front()->jumped;
			// Drop Item can only be cancelled if item is in agent's inventory
			case BattleUnitMission::Type::DropItem:
				if (missions.front()->item && !missions.front()->item->ownerAgent)
				{
					return false;
				}
				break;
			// Missions that must be let finish
			case BattleUnitMission::Type::ChangeBodyState:
			case BattleUnitMission::Type::Turn:
			case BattleUnitMission::Type::GotoLocation:
			case BattleUnitMission::Type::ReachGoal:
				letFinish = true;
				break;
			// Missions that can be cancelled
			case BattleUnitMission::Type::Snooze:
			case BattleUnitMission::Type::Teleport:
			case BattleUnitMission::Type::RestartNextMission:
			case BattleUnitMission::Type::AcquireTU:
				break;
		}

		// Figure out what to do with the unfinished mission
		if (letFinish)
		{
			auto &m = missions.front();
			// If turning - downgrade to a turning mission
			if (facing != goalFacing)
			{
				m->type = BattleUnitMission::Type::Turn;
				m->targetFacing = goalFacing;
				if (m->costPaidUpFront > 0)
				{
					// Refund queued action, subtract turning cost
					agent->modified_stats.time_units += m->costPaidUpFront - 1;
				}
			}
			// If changing body - downgrade to a body state change mission
			else if (current_body_state != target_body_state)
			{
				m->type = BattleUnitMission::Type::ChangeBodyState;
				m->targetBodyState = target_body_state;
			}
			else
			{
				facingDelta = m->facingDelta;
				letFinish = false;
			}
		}

		// Cancel missions
		while (missions.size() > (letFinish ? 1 : 0))
		{
			agent->modified_stats.time_units += missions.back()->costPaidUpFront;
			missions.pop_back();
		}
		if (missions.empty() && !atGoal)
		{
			addMission(state, BattleUnitMission::reachGoal(*this, facingDelta));
		}
	}
	return true;
}

bool BattleUnit::setMission(GameState &state, BattleUnitMission *mission)
{
	// Check if mission was actually passed
	// We can receive nullptr here in case mission was impossible
	if (!mission)
	{
		return false;
	}

	// Special checks and actions based on mission type
	switch (mission->type)
	{
		case BattleUnitMission::Type::Jump:
		case BattleUnitMission::Type::Turn:
			stopAttacking();
			break;
		default:
			// Nothing to check for in other missions
			break;
	}

	if (!cancelMissions(state))
	{
		delete mission;
		return false;
	}

	// There is a mission remaining that wants to let it finish
	if (!missions.empty())
	{
		switch (mission->type)
		{
			// Brainsuck overwhelms everything
			case BattleUnitMission::Type::Brainsuck:
			{
				setMovementState(MovementState::None);
				setBodyState(state, BodyState::Standing);
				setFacing(state, goalFacing);
				missions.clear();
				break;
			}
			// Instant throw always cancels if agent can afford it
			case BattleUnitMission::Type::ThrowItem:
			{
				// FIXME: actually read the option
				if (!config().getBool("OpenApoc.NewFeature.NoInstantThrows") &&
				    canAfford(state, getThrowCost(), true))
				{
					setMovementState(MovementState::None);
					setBodyState(state, BodyState::Standing);
					setFacing(state, BattleUnitMission::getFacing(*this, mission->targetLocation));
					missions.clear();
				}
				break;
			}
			// Turning can be cancelled if our mission will require us to turn in a different dir
			// Also reachGoal can be cancelled by GotoLocation
			case BattleUnitMission::Type::Turn:
			case BattleUnitMission::Type::GotoLocation:
			case BattleUnitMission::Type::ReachGoal:
			{
				if (missions.front()->type == BattleUnitMission::Type::ReachGoal &&
				    mission->type == BattleUnitMission::Type::GotoLocation)
				{
					missions.clear();
				}
				else if (facing != goalFacing)
				{
					Vec2<int> nextFacing;
					bool haveNextFacing = true;
					switch (mission->type)
					{
						case BattleUnitMission::Type::Turn:
							nextFacing =
							    BattleUnitMission::getFacingStep(*this, mission->targetFacing);
							break;
						case BattleUnitMission::Type::GotoLocation:
							// We have to start it in order to see where we're going
							mission->start(state, *this);
							if (mission->currentPlannedPath.empty())
							{
								haveNextFacing = false;
								break;
							}
							nextFacing = BattleUnitMission::getFacingStep(
							    *this, BattleUnitMission::getFacing(
							               *this, mission->currentPlannedPath.front()));
							break;
						case BattleUnitMission::Type::ReachGoal:
							nextFacing = BattleUnitMission::getFacingStep(
							    *this, BattleUnitMission::getFacing(*this, position, goalPosition));
							break;
						default: // don't cry about unhandled case, compiler
							break;
					}
					// If we are turning towards something that will not be our next facing when we
					// try
					// to execute our mission then we're better off canceling it
					if (haveNextFacing && nextFacing != goalFacing)
					{
						setFacing(state, facing);
						missions.clear();
					}
				}
				break;
			}
			default:
				break;
		}
	}

	// Finally, add the mission
	return addMission(state, mission);
}

bool BattleUnit::addMission(GameState &state, BattleUnitMission *mission, bool toBack)
{
	if (toBack)
	{
		missions.emplace_back(mission);
		return true;
	}

	switch (mission->type)
	{
		// Reach goal can only be added if it can overwrite the mission
		case BattleUnitMission::Type::ReachGoal:
		{
			if (missions.size() > 0)
			{
				switch (missions.front()->type)
				{
					// Missions that prevent going to goal
					case BattleUnitMission::Type::Snooze:
					case BattleUnitMission::Type::Jump:
					case BattleUnitMission::Type::ThrowItem:
					case BattleUnitMission::Type::ChangeBodyState:
					case BattleUnitMission::Type::ReachGoal:
					case BattleUnitMission::Type::DropItem:
					case BattleUnitMission::Type::Teleport:
					case BattleUnitMission::Type::RestartNextMission:
					case BattleUnitMission::Type::Brainsuck:
					case BattleUnitMission::Type::GotoLocation:
					case BattleUnitMission::Type::Turn:
						delete mission;
						return false;
					// Missions that can be overwritten
					case BattleUnitMission::Type::AcquireTU:
						break;
				}
			}
			missions.emplace_front(mission);
			mission->start(state, *this);
			break;
		}
		// Missions that can be added to the back at any time
		case BattleUnitMission::Type::Turn:
		case BattleUnitMission::Type::ChangeBodyState:
		case BattleUnitMission::Type::ThrowItem:
		case BattleUnitMission::Type::GotoLocation:
		case BattleUnitMission::Type::Jump:
		case BattleUnitMission::Type::Teleport:
			missions.emplace_back(mission);
			// Missions added to back normally start only if they are the only mission in the queue
			// Teleport always starts immediately, even if the agent is waiting to finish something
			if (missions.size() == 1 || mission->type == BattleUnitMission::Type::Teleport)
			{
				mission->start(state, *this);
			}
			break;
		// Missions that can be added to the front at any time
		case BattleUnitMission::Type::Snooze:
		case BattleUnitMission::Type::AcquireTU:
		case BattleUnitMission::Type::RestartNextMission:
		case BattleUnitMission::Type::DropItem:
		case BattleUnitMission::Type::Brainsuck:
			missions.emplace_front(mission);
			mission->start(state, *this);
			break;
	}
	return !mission->cancelled;
}
} // namespace OpenApoc
