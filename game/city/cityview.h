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

enum class CityIcon
{
	UnselectedFrame,
	SelectedFrame,
	SelectedSecondaryFrame,

	InBase,
	InVehicle,
	InBuilding,
	InMotion,
};

class CityView : public TileView
{
  private:
	sp<Form> activeTab, baseForm;
	std::vector<sp<Form>> uiTabs;
	UpdateSpeed updateSpeed;

	sp<GameState> state;
	std::map<CityIcon, sp<Image>> icons;

	std::map<Control *, wp<Vehicle>> playerVehicleListControls;

	wp<Vehicle> selectedVehicle;

	// We use a scaled image to implement the health bar
	sp<Image> healthImage;
	sp<Image> shieldImage;

  public:
	CityView(sp<GameState> state);
	virtual ~CityView();
	virtual void Update(StageCmd *const cmd) override;
	virtual void Render() override;
	virtual void EventOccurred(Event *e) override;
};

}; // namespace OpenApoc
