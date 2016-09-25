#pragma once
#include "game/ui/tileview/battletileview.h"
#include "library/sp.h"
#include "library/colour.h"

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
	NormalCtrlAltShift,
	Fire,

	// Throw,
	// Psi,
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
	const std::vector<Colour> accuracyColors =
	{
		{ 190, 36,36 },
		{ 203 ,28 ,2},
		{ 235 ,77 ,4},
		{ 239 ,125,52},
		{ 243 ,170,85},
		{ 247 ,219,117 },
		{ 235 ,247,138 },
	};
	const Colour ammoColour = { 158, 24, 12 };

	sp<Form> activeTab, baseForm;
	std::vector<sp<Form>> uiTabsRT;
	std::vector<sp<Form>> uiTabsTB;
	BattleUpdateSpeed updateSpeed;
	BattleUpdateSpeed lastSpeed;

	sp<GameState> state;

	AgentEquipmentInfo leftHandInfo;
	AgentEquipmentInfo rightHandInfo;

	bool followAgent;

	bool colorForward = true;
	int colorCurrent = 0;
	sp<Palette> palette;

	BattleSelectionState selectionState;
	bool ModifierLShift = false;
	bool ModifierRShift = false;
	bool ModifierLAlt = false;
	bool ModifierRAlt = false;
	bool ModifierLCtrl = false;
	bool ModifierRCtrl = false;

	void updateSelectionMode();
	void updateSelectedUnits();
	void updateLayerButtons();
	void updateSoldierButtons();

	AgentEquipmentInfo createItemOverlayInfo(bool rightHand);
	void updateItemInfo(bool right);
	sp<Image> selectedItemOverlay;


	// Unit orers
	// Move, offset 1 means strafing, 2 means move backwards
	void orderMove(Vec3<int> target, int facingOffset = 0, bool demandGiveWay = false);
	void orderTurn(Vec3<int> target);
	void orderSelect(sp<BattleUnit> u, bool inverse = false, bool additive = false);
	void attemptToClearCurrentOrders(sp<BattleUnit> u);
	bool canEmplaceTurnInFront(sp<BattleUnit> u);

  public:
	BattleView(sp<GameState> state);
	~BattleView() override;
	void begin() override;
	void resume() override;
	void update() override;
	void render() override;
	void eventOccurred(Event *e) override;

	void setUpdateSpeed(BattleUpdateSpeed updateSpeed);
};



}; // namespace OpenApoc
