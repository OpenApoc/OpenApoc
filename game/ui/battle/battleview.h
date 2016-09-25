#pragma once
#include "game/ui/tileview/battletileview.h"
#include "library/sp.h"

namespace OpenApoc
{

class Form;
class GameState;
class GraphicButton;
class Control;

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

class BattleView : public BattleTileView
{
  private:
	sp<Form> activeTab, baseForm;
	std::vector<sp<Form>> uiTabsRT;
	std::vector<sp<Form>> uiTabsTB;
	BattleUpdateSpeed updateSpeed;
	BattleUpdateSpeed lastSpeed;

	void setUpdateSpeed(BattleUpdateSpeed updateSpeed);

	sp<GameState> state;

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
};

}; // namespace OpenApoc
