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

class BattleUnitAnimation : public StateObject<BattleUnitAnimation>
{
  public:
	enum class AnimationDirection
	{
		Into,
		Within,
		Back
	};
	enum class ItemInHands
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
			};

			InfoBlock shadow;
			InfoBlock body;
			InfoBlock leg;
			InfoBlock helmet;
			InfoBlock left_arm;
			InfoBlock right_arm;
		};

		std::vector<Frame> frames;
		int frame_count = 0;
	};

	// Animations defined here

	std::map<AnimationDirection,
	         std::map<BattleUnit::Stance,
	                  std::map<BattleUnit::HandState,
	                           std::map<Vec2<int>, std::map<ItemInHands, sp<AnimationEntry>>>>>>
	    animations;

	// Animation for busy hands is only present for static states
	// To get an animation with busy hands for a non-static state, legs are used from corresponding
	// non-static state while everything else is used from static state

	// Into and Back animations are only present for kneeling/prone and aiming.
	// Kneeling can be done only from standing, and prone can be done only from kneeling.
	// Backwards is the same, back from prone gets you to kneeling, back from kneeling gets you to
	// standing
	// So, for example, transition from prone to standing is "Back" of prone and then "Back" of
	// kneeling banded together

	// FIXME: Implement these

	int getFrameCount(StateRef<AEquipmentType> heldItem, Vec2<int> facing,
	                  BattleUnit::Stance currentStance, BattleUnit::Stance targetStance,
	                  BattleUnit::HandState currentHandState, BattleUnit::HandState targetHandState,
	                  bool moving);

	void drawShadow(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	                TileViewMode mode, StateRef<BattleUnitImagePack> shadow,
	                StateRef<AEquipmentType> heldItem, Vec2<int> facing,
	                BattleUnit::Stance currentStance, BattleUnit::Stance targetStance,
	                BattleUnit::HandState currentHandState, BattleUnit::HandState targetHandState,
	                bool moving, int frame);

	void drawUnit(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	              TileViewMode mode, StateRef<BattleUnitImagePack> body,
	              StateRef<BattleUnitImagePack> legs, StateRef<BattleUnitImagePack> helmet,
	              StateRef<BattleUnitImagePack> leftHand, StateRef<BattleUnitImagePack> rightHand,
	              StateRef<AEquipmentType> heldItem, Vec2<int> facing,
	              BattleUnit::Stance currentStance, BattleUnit::Stance targetStance,
	              BattleUnit::HandState currentHandState, BattleUnit::HandState targetHandState,
	              bool moving, int frame);
};
}
