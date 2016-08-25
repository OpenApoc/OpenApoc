#pragma once
#include "game/ui/battletileview/battletileview.h"
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
		Move,
		Throw,
		Fire,
	};

	class BattleView: public BattleTileView
	{
	private:
		sp<Form> activeTab, baseForm;
		std::vector<sp<Form>> uiTabs;
		BattleUpdateSpeed updateSpeed;

		void setUpdateSpeed(BattleUpdateSpeed updateSpeed);

		sp<GameState> state;

		bool followAgent;

		sp<Palette> palette;

		BattleSelectionState selectionState;

	public:
		BattleView(sp<GameState> state);
		~BattleView() override;
		void begin() override;
		void resume() override;
		void update(StageCmd *const cmd) override;
		void render() override;
		void eventOccurred(Event *e) override;
	};

}; // namespace OpenApoc
