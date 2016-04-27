#pragma once
#include "game/ui/tileview/tileview.h"
#include "library/sp.h"

namespace OpenApoc
{

class Form;
class GameState;
class GraphicButton;
class Control;

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

enum class CityUnitState
{
	InBase,
	InVehicle,
	InBuilding,
	InMotion,
};

enum class SelectionState
{
	Normal,
	VehicleGotoBuilding,
	VehicleGotoLocation,
	VehicleAttackVehicle,
	VehicleAttackBuilding,
	BuildBase,
};

// All the info required to draw a single vehicle info chunk, kept together to make it easier to
// track when something has changed and requires a re-draw
class VehicleTileInfo
{
  public:
	sp<Vehicle> vehicle;
	bool selected;
	float healthProportion;
	bool shield;
	bool faded;     // Faded when they enter the alien dimension?
	int passengers; // 0-13, 0-12 having numbers, 13+ being '+'
	CityUnitState state;
	bool operator==(const VehicleTileInfo &other) const;
};

class CityView : public TileView
{
  private:
	sp<Form> activeTab, baseForm;
	std::vector<sp<Form>> uiTabs;
	std::vector<sp<GraphicButton>> miniViews;
	UpdateSpeed updateSpeed;

	sp<GameState> state;
	StateRef<City> city;
	StateRef<Base> base;
	std::map<CityIcon, sp<Image>> icons;

	std::vector<sp<Image>> vehiclePassengerCountIcons;

	std::map<sp<Vehicle>, std::pair<VehicleTileInfo, sp<Control>>> vehicleListControls;

	wp<Vehicle> selectedVehicle;

	// We use a scaled image to implement the health bar
	sp<Image> healthImage;
	sp<Image> shieldImage;

	bool followVehicle;

	VehicleTileInfo createVehicleInfo(sp<Vehicle> v);
	sp<Control> createVehicleInfoControl(const VehicleTileInfo &info);

	SelectionState selectionState;

  public:
	CityView(sp<GameState> state, StateRef<City> city);
	virtual ~CityView();
	void Resume() override;
	void Update(StageCmd *const cmd) override;
	void Render() override;
	void EventOccurred(Event *e) override;
};

}; // namespace OpenApoc
