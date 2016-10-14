#pragma once
#include "game/state/battle/battleunit.h"
#include "game/ui/tileview/battletileview.h"
#include "library/colour.h"
#include "library/sp.h"

namespace OpenApoc
{

class Form;
class GameState;
class GraphicButton;
class Control;
class AEquipment;
class AEquipmentType;

enum class BattleUpdateSpeed
{
	Pause,
	Speed1,
	Speed2,
	Speed3,
};

enum class BattleSelectionState
{
	Normal,
	NormalAlt,
	NormalCtrl,
	NormalCtrlAlt,
	PsiLeft,
	PsiRight,
	FireAny,
	FireLeft,
	FireRight,
	ThrowLeft,
	ThrowRight,
	TeleportLeft,
	TeleportRight,
};

// All the info required to draw a single item info chunk, kept together to make it easier to
// track when something has changed and requires a re-draw
class AgentEquipmentInfo
{
  public:
	StateRef<AEquipmentType> itemType;
	StateRef<DamageType> damageType;
	bool selected = false;
	int curAmmo = 0;
	int maxAmmo = 0;
	int accuracy = 0;
	bool operator==(const AgentEquipmentInfo &other) const;
};

class BattleView : public BattleTileView
{
  private:
	const std::vector<Colour> accuracyColors = {
	    {190, 36, 36},  {203, 28, 2},    {235, 77, 4},    {239, 125, 52},
	    {243, 170, 85}, {247, 219, 117}, {235, 247, 138},
	};
	const Colour ammoColour = {158, 24, 12};

	sp<Form> activeTab, mainTab, psiTab, primingTab, baseForm;
	std::vector<sp<Form>> uiTabsRT;
	std::vector<sp<Form>> uiTabsTB;
	BattleUpdateSpeed updateSpeed;
	BattleUpdateSpeed lastSpeed;

	sp<GameState> state;

	AgentEquipmentInfo leftHandInfo;
	AgentEquipmentInfo rightHandInfo;

	bool followAgent = false;

	bool colorForward = true;
	int colorCurrent = 0;
	sp<Palette> palette;
	std::vector<sp<Palette>> modPalette;

	BattleSelectionState selectionState;
	bool modifierLShift = false;
	bool modifierRShift = false;
	bool modifierLAlt = false;
	bool modifierRAlt = false;
	bool modifierLCtrl = false;
	bool modifierRCtrl = false;
	int leftThrowDelay = 0;
	int rightThrowDelay = 0;
	int actionImpossibleDelay = 0;

	void updateSelectionMode();
	void updateSelectedUnits();
	void updateLayerButtons();
	void updateSoldierButtons();

	AgentEquipmentInfo createItemOverlayInfo(bool rightHand);
	void updateItemInfo(bool right);
	sp<Image> selectedItemOverlay;

	sp<Image> pauseIcon;
	int pauseIconTimer = 0;

	void onNewTurn();

	// Unit orers

	void orderMove(Vec3<int> target, bool strafe = false, bool demandGiveWay = false);
	void orderTurn(Vec3<int> target);
	void orderUse(bool right, bool automatic);
	void orderDrop(bool right);
	void orderThrow(Vec3<int> target, bool right);
	void orderTeleport(Vec3<int> target, bool right);
	void orderSelect(sp<BattleUnit> u, bool inverse = false, bool additive = false);
	void orderFire(Vec3<int> target,
	               BattleUnit::WeaponStatus status = BattleUnit::WeaponStatus::FiringBothHands,
	               bool modifier = false);
	void orderFire(StateRef<BattleUnit> u,
	               BattleUnit::WeaponStatus status = BattleUnit::WeaponStatus::FiringBothHands);
	void orderFocus(StateRef<BattleUnit> u);

	void attemptToClearCurrentOrders(sp<BattleUnit> u, bool overrideBodyStateChange = false);
	bool canEmplaceTurnInFront(sp<BattleUnit> u);

  public:
	BattleView(sp<GameState> state);
	~BattleView() override;
	void begin() override;
	void resume() override;
	void update() override;
	void render() override;
	void finish() override;
	void eventOccurred(Event *e) override;

	void setUpdateSpeed(BattleUpdateSpeed updateSpeed);
};

}; // namespace OpenApoc
