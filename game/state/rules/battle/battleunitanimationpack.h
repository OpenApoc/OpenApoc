#pragma once

#include "game/state/rules/aequipmenttype.h"
#include "game/state/shared/agent.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>
#include <vector>

namespace OpenApoc
{

class Renderer;
enum class TileViewMode;
class TileTransform;
class BattleUnitImagePack;

enum class ItemWieldMode
{
	None,
	OneHanded,
	TwoHanded
};

class BattleUnitAnimationPack : public StateObject<BattleUnitAnimationPack>
{
  public:
	class AnimationEntry
	{
	  public:
		class Frame
		{
		  public:
			class InfoBlock
			{
			  public:
				int index = -1;
				Vec2<float> offset;
				InfoBlock() = default;
				InfoBlock(int index, int offset_x, int offet_y);
			};

			enum class UnitImagePart
			{
				Shadow,
				Body,
				Legs,
				Helmet,
				LeftArm,
				RightArm,
				Weapon
			};

			std::map<UnitImagePart, InfoBlock> unit_image_parts;
			// When drawing, go through this list in forward order and draw each part referenced
			std::list<UnitImagePart> unit_image_draw_order;
		};

		std::vector<Frame> frames;
		// If set, will require addition of legs from "at ease" animation with same parameters
		bool is_overlay = false;
		// Just for convenience
		int frame_count = 0;
		// Amount of ingame units that correspond to 100 frames of this animation
		// The bigger the number, the slower the animation is played
		// Nominated in case we have non-integer amount of units per frame
		int units_per_100_frames = 0;
	};

	class AnimationKey
	{
	  public:
		ItemWieldMode itemWieldMode = ItemWieldMode::None;
		HandState handState = HandState::AtEase;
		MovementState movementState = MovementState::None;
		BodyState bodyState = BodyState::Standing;
		AnimationKey() = default;
		AnimationKey(ItemWieldMode itemWieldMode, HandState handState, MovementState movementState,
		             BodyState bodyState)
		    : itemWieldMode(itemWieldMode), handState(handState), movementState(movementState),
		      bodyState(bodyState)
		{
		}
		bool operator<(const AnimationKey &other) const
		{
			return std::tie(itemWieldMode, handState, movementState, bodyState) <
			       std::tie(other.itemWieldMode, other.handState, other.movementState,
			                other.bodyState);
		}
	};

	// Animations for cases where current state is equal to target state
	std::map<AnimationKey, std::map<Vec2<int>, sp<AnimationEntry>>> standart_animations;

	class ChangingHandAnimationKey
	{
	  public:
		ItemWieldMode itemWieldMode = ItemWieldMode::None;
		HandState currentHand = HandState::AtEase;
		HandState targetHand = HandState::AtEase;
		MovementState movementState = MovementState::None;
		BodyState bodyState = BodyState::Standing;
		ChangingHandAnimationKey() = default;
		ChangingHandAnimationKey(ItemWieldMode itemWieldMode, HandState currentHand,
		                         HandState targetHand, MovementState movementState,
		                         BodyState bodyState)
		    : itemWieldMode(itemWieldMode), currentHand(currentHand), targetHand(targetHand),
		      movementState(movementState), bodyState(bodyState)
		{
		}
		bool operator<(const ChangingHandAnimationKey &other) const
		{
			return std::tie(itemWieldMode, currentHand, targetHand, movementState, bodyState) <
			       std::tie(other.itemWieldMode, other.currentHand, other.targetHand,
			                other.movementState, other.bodyState);
		}
	};

	// Animation for changing hand state. First is current, second is target.
	std::map<ChangingHandAnimationKey, std::map<Vec2<int>, sp<AnimationEntry>>>
	    hand_state_animations;

	class ChangingBodyStateAnimationKey
	{
	  public:
		ItemWieldMode itemWieldMode = ItemWieldMode::None;
		HandState handState = HandState::AtEase;
		MovementState movementState = MovementState::None;
		BodyState currentBodyState = BodyState::Standing;
		BodyState targetBodyState = BodyState::Standing;
		ChangingBodyStateAnimationKey() = default;
		ChangingBodyStateAnimationKey(ItemWieldMode itemWieldMode, HandState handState,
		                              MovementState movementState, BodyState currentBodyState,
		                              BodyState targetBodyState)
		    : itemWieldMode(itemWieldMode), handState(handState), movementState(movementState),
		      currentBodyState(currentBodyState), targetBodyState(targetBodyState)
		{
		}
		bool operator<(const ChangingBodyStateAnimationKey &other) const
		{
			return std::tie(itemWieldMode, handState, movementState, currentBodyState,
			                targetBodyState) < std::tie(other.itemWieldMode, other.handState,
			                                            other.movementState, other.currentBodyState,
			                                            other.targetBodyState);
		}
	};
	// Animation for changing body state. First is current, second is target.
	std::map<ChangingBodyStateAnimationKey, std::map<Vec2<int>, sp<AnimationEntry>>>
	    body_state_animations;

	// Whether unit has alternative firing animations - upwards and downwards (2 versions each)
	bool hasAlternativeFiringAnimations = false;

	// Whether unit should play it's firing animation when using psionics
	bool useFiringAnimationForPsi = false;

	// Animation for alternative aiming/firing. HandState must be "Firing" or "Aiming",
	// second parameter is firing angle, which can be +/-1 or +/-2
	// where + is firing upwards and - is downwards,
	// 1 is angles 10-20 (degrees) and 2 is 20 and further
	class AltFireAnimationKey
	{
	  public:
		ItemWieldMode itemWieldMode = ItemWieldMode::None;
		HandState handState = HandState::Aiming;
		int angle = 0;
		MovementState movementState = MovementState::None;
		BodyState bodyState = BodyState::Standing;
		AltFireAnimationKey() = default;

		AltFireAnimationKey(ItemWieldMode itemWieldMode, HandState handState, int angle,
		                    MovementState movementState, BodyState bodyState)
		    : itemWieldMode(itemWieldMode), handState(handState), angle(angle),
		      movementState(movementState), bodyState(bodyState)
		{
		}
		bool operator<(const AltFireAnimationKey &other) const
		{
			return std::tie(itemWieldMode, handState, angle, movementState, bodyState) <
			       std::tie(other.itemWieldMode, other.handState, other.angle, other.movementState,
			                other.bodyState);
		}
	};
	std::map<AltFireAnimationKey, std::map<Vec2<int>, sp<AnimationEntry>>> alt_fire_animations;

	// Animation functions

	// Get frame count for animation of body change. 0 means there's no animation present
	int getFrameCountBody(StateRef<AEquipmentType> heldItem, BodyState currentBody,
	                      BodyState targetBody, HandState currentHands, MovementState movement,
	                      Vec2<int> facing);

	// Get frame count for animation of hand change. 0 means there's no animation present
	int getFrameCountHands(StateRef<AEquipmentType> heldItem, BodyState currentBody,
	                       HandState currentHands, HandState targetHands, MovementState movement,
	                       Vec2<int> facing);

	// Get frame count for animation of hand change. 0 means there's no animation present
	int getFrameCountFiring(StateRef<AEquipmentType> heldItem, BodyState currentBody,
	                        MovementState movement, Vec2<int> facing);

	// Draw unit's shadow
	void drawShadow(Renderer &r, Vec2<float> screenPosition, StateRef<BattleUnitImagePack> shadow,
	                StateRef<AEquipmentType> heldItem, Vec2<int> facing, BodyState currentBody,
	                BodyState targetBody, HandState currentHands, HandState targetHands,
	                MovementState movement, int body_animation_delay, int hands_animation_delay,
	                int distance_travelled, bool visible);

	// Draw unit's body and held item
	// firingAngle is 0 for 0-15 degrees, 1 for 15-30 degrees and 2 for 30+ degrees, positive is up
	void drawUnit(Renderer &r, Vec2<float> screenPosition, StateRef<BattleUnitImagePack> body,
	              StateRef<BattleUnitImagePack> legs, StateRef<BattleUnitImagePack> helmet,
	              StateRef<BattleUnitImagePack> leftHand, StateRef<BattleUnitImagePack> rightHand,
	              StateRef<AEquipmentType> heldItem, Vec2<int> facing, BodyState currentBody,
	              BodyState targetBody, HandState currentHands, HandState targetHands,
	              MovementState movement, int body_animation_delay, int hands_animation_delay,
	              int distance_travelled, int firingAngle, bool visible, bool stealth);

	// high level api for loading
	bool loadAnimationPack(GameState &state, const UString &path);

	// high level api for saving
	bool saveAnimationPack(const UString &path, bool pack = true, bool pretty = false);

	// Function used when getting file path
	static const UString getNameFromID(UString id);

	static UString getAnimationPackPath();
};
} // namespace OpenApoc
