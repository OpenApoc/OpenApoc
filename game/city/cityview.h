#pragma once
#include "library/sp.h"

#include "game/tileview/tileview.h"

namespace OpenApoc
{

class Form;
class GameState;

enum class UpdateSpeed
{
	Pause,
	Speed1,
	Speed2,
	Speed3,
	Speed4,
	Speed5,
};

class CityView : public TileView
{
  private:
	sp<Form> activeTab;
	std::vector<sp<Form>> uiTabs;
	UpdateSpeed updateSpeed;

	sp<GameState> state;

  public:
	CityView(sp<GameState> state);
	virtual ~CityView();
	virtual void Update(StageCmd *const cmd) override;
	virtual void Render() override;
	virtual void EventOccurred(Event *e) override;
};

}; // namespace OpenApoc
