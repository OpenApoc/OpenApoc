#pragma once
#include "game/state/agent.h"
#include "game/state/battle/battleunit.h"
#include "game/state/battle/battleunitimagepack.h"
#include "game/state/rules/aequipment_type.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"

namespace OpenApoc
{

class Renderer;
enum class TileViewMode;
class TileTransform;

class BattleUnitAnimationPack : public StateObject<BattleUnitAnimationPack>
{
  public:
	enum class ItemWieldMode
	{
		None,
		OneHanded,
		TwoHanded
	};

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
			// here
			std::list<UnitImagePart> unit_image_draw_order;
		};

		std::vector<Frame> frames;
		// If set, will require addition of legs from "at ease" animation with same parameters
		bool is_overlay = false;
		// Just for convenience
		int frame_count = 0;
		// In case we have non-integer amount of frames per unit
		int frames_per_100_units = 0;
	};

	// Animations for cases where current state is equal to target state
	std::map<ItemWieldMode,
	    std::map<AgentType::HandState,
			std::map<AgentType::MovementState,
				std::map<AgentType::BodyState, std::map<Vec2<int>, sp<AnimationEntry>>>>>>
	    standart_animations;

	// Animation for changing hand state. First is current, second is target.
	std::map<ItemWieldMode,
		std::map<AgentType::HandState,
			std::map<AgentType::HandState,
				std::map<AgentType::MovementState,
					std::map<AgentType::BodyState,
						std::map<Vec2<int>, sp<AnimationEntry>>>>>>>
	    hand_state_animations;

	// Animation for changing body state. First is current, second is target.
	std::map<ItemWieldMode,
		std::map<AgentType::HandState,
			std::map<AgentType::MovementState,
				std::map<AgentType::BodyState,
					std::map<AgentType::BodyState,
						std::map<Vec2<int>, sp<AnimationEntry>>>>>>>
	    body_state_animations;

	// Wether unit has alternative firing animations - upwards and downwards (2 versions each)
	bool hasAlternativeFiringAnimations = false;
	
	// Animation for alternative firing. HandState must be "Firing", 
	// second parameter is firing angle, which can be +/-1 or +/-2
	// where + is firing upwards and - is downwards,
	// 1 is angles 15-30 (degrees) and 2 is 30 and further
	std::map<ItemWieldMode,
		std::map<int,
			std::map<AgentType::MovementState,
				std::map<AgentType::BodyState,
					std::map<Vec2<int>, sp<AnimationEntry>>>>>>
						alt_fire_animations;

	// Animation functions

	// Get frame count for animation of body change. 0 means there's no animation present
	int getFrameCountBody(StateRef<AEquipmentType> heldItem, AgentType::BodyState currentBody,
	                      AgentType::BodyState targetBody, AgentType::HandState currentHands,
	                      AgentType::MovementState movement, Vec2<int> facing);

	// Get frame count for animation of hand change. 0 means there's no animation present
	int getFrameCountHands(StateRef<AEquipmentType> heldItem, AgentType::BodyState currentBody,
	                       AgentType::HandState currentHands, AgentType::HandState targetHands,
	                       AgentType::MovementState movement, Vec2<int> facing);

	// Draw unit's shadow
	void drawShadow(Renderer &r, Vec2<float> screenPosition, StateRef<BattleUnitImagePack> shadow,
	                StateRef<AEquipmentType> heldItem, Vec2<int> facing,
	                AgentType::BodyState currentBody, AgentType::BodyState targetBody,
	                AgentType::HandState currentHands, AgentType::HandState targetHands,
	                AgentType::MovementState movement, int body_animation_delay,
	                int hands_animation_delay, int distance_travelled);

	// Draw unit's body and held item
	// firingAngle is 0 for 0-15 degrees, 1 for 15-30 degrees and 2 for 30+ degrees, positive is up
	void drawUnit(Renderer &r, Vec2<float> screenPosition, StateRef<BattleUnitImagePack> body,
	              StateRef<BattleUnitImagePack> legs, StateRef<BattleUnitImagePack> helmet,
	              StateRef<BattleUnitImagePack> leftHand, StateRef<BattleUnitImagePack> rightHand,
	              StateRef<AEquipmentType> heldItem, Vec2<int> facing,
	              AgentType::BodyState currentBody, AgentType::BodyState targetBody,
	              AgentType::HandState currentHands, AgentType::HandState targetHands,
	              AgentType::MovementState movement, int body_animation_delay,
	              int hands_animation_delay, int distance_travelled, int firingAngle = 0);

	// high level api for loading
	bool loadAnimationPack(GameState &state, const UString &path);

	// high level api for saving
	bool saveAnimationPack(const UString &path, bool pack = true);

	// Function used when getting file path
	static const UString getNameFromID(UString id);

	static const UString animationPackPath;
};
}
