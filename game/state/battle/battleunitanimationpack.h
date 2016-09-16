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
				int index = 0;
				int offset_x = 0;
				int offset_y = 0;
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
			// When drawing, go through this list in forward order and draw each part referenced here
			std::list<UnitImagePart> unit_image_draw_order;
		};

		// If set, will require addition of legs from "at ease" animation with same parameters
		bool is_overlay = false;
		std::vector<Frame> frames;
		int frame_count = 0;
	};

	// Animations for cases where current state is equal to target state
	std::map<ItemWieldMode,
		std::map<AgentType::HandState,
		std::map<AgentType::MovementState,
		std::map<AgentType::BodyState,
		std::map<Vec2<int>, sp<AnimationEntry>>>>>>
		standart_animations;

	// Animation for changing hand state. First is current, second is target.
	// MovementState::None = Stationary, MovementState::Normal = Overlay animations
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

	// FIXME: Implement these

	int getFrameCountBody(StateRef<AEquipmentType> heldItem,
	                AgentType::BodyState currentBody, AgentType::BodyState targetBody,
					AgentType::HandState currentHands,
					AgentType::MovementState movement,
					Vec2<int> direction);

	int getFrameCountHands(StateRef<AEquipmentType> heldItem,
					AgentType::BodyState currentBody, 
					AgentType::HandState currentHands, AgentType::HandState targetHands,
					AgentType::MovementState movement,
					Vec2<int> direction);

	void drawShadow(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	                TileViewMode mode, StateRef<BattleUnitImagePack> shadow,
	                StateRef<AEquipmentType> heldItem, Vec2<int> facing,
					AgentType::BodyState currentBody, AgentType::BodyState targetBody,
					AgentType::HandState currentHands, AgentType::HandState targetHands,
					AgentType::MovementState movement,
	                int body_animation_delay, int hands_animation_delay, int distance_passed);

	void drawUnit(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	                TileViewMode mode, StateRef<BattleUnitImagePack> body,
	                StateRef<BattleUnitImagePack> legs, StateRef<BattleUnitImagePack> helmet,
	                StateRef<BattleUnitImagePack> leftHand, StateRef<BattleUnitImagePack> rightHand,
	                StateRef<AEquipmentType> heldItem, Vec2<int> facing,
					AgentType::BodyState currentBody, AgentType::BodyState targetBody,
					AgentType::HandState currentHands, AgentType::HandState targetHands,
					AgentType::MovementState movement,
					int body_animation_delay, int hands_animation_delay, int distance_passed);

	// high level api for loading
	bool loadAnimationPack(GameState &state, const UString &path);

	// high level api for saving
	bool saveAnimationPack(const UString &path, bool pack = true);

	// Function used when getting file path
	static const UString getNameFromID(UString id);

	static const UString animationPackPath;
};
}
