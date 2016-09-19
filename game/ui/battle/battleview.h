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
	Fire,
	
	//Throw,
	//Psi,
};

class BattleView : public BattleTileView
{
  private:
	sp<Form> activeTab, baseForm;
	std::vector<sp<Form>> uiTabs;
	BattleUpdateSpeed updateSpeed;
	BattleUpdateSpeed lastSpeed;

	void setUpdateSpeed(BattleUpdateSpeed updateSpeed);

	sp<GameState> state;

	std::list<sp<BattleUnit>> selectedUnits;

	bool followAgent;

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
