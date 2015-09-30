#pragma once

#include "game/tileview/tileview.h"

namespace OpenApoc
{

class Form;

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
	std::shared_ptr<Form> activeTab;
	std::vector<std::shared_ptr<Form>> uiTabs;
	UpdateSpeed updateSpeed;

  public:
	CityView(Framework &fw);
	virtual ~CityView();
	virtual void Update(StageCmd *const cmd) override;
	virtual void Render() override;
	virtual void EventOccurred(Event *e) override;
};

}; // namespace OpenApoc
