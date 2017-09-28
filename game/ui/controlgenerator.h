#pragma once
#include "library/sp.h"
#include "library/vec.h"
#include <vector>

namespace OpenApoc
{

class Vehicle;
class Agent;
class GameState;
class VehicleTileInfo;
class AgentInfo;
class Image;
class BitmapFont;
class Control;

class ControlGenerator
{
  private:
	static ControlGenerator singleton;
	void init(GameState &state);
	bool initialised = false;

	sp<BitmapFont> labelFont;
	sp<Image> healthImage;
	sp<Image> shieldImage;
	sp<Image> stunImage;
	sp<Image> iconShade;
	std::vector<sp<Image>> unitSelect;
	std::vector<sp<Image>> vehiclePassengerCountIcons;
	std::vector<sp<Image>> icons;

  public:
	static VehicleTileInfo createVehicleInfo(GameState &state, sp<Vehicle> v);
	static sp<Control> createVehicleControl(GameState &state, const VehicleTileInfo &info);
	static sp<Control> createVehicleControl(GameState &state, sp<Vehicle> v);

};

enum class CityUnitState
{
	InBase = 0,
	InVehicle = 1,
	InBuilding = 2,
	InMotion = 3,
};

// All the info required to draw a single vehicle info chunk, kept together to make it easier to
// track when something has changed and requires a re-draw
class VehicleTileInfo
{
public:
	sp<Vehicle> vehicle;
	// 0 = not selected, 1 = selected, 2 = first selected
	int selected;
	float healthProportion;
	bool shield;
	bool faded;     // Faded when they enter the alien dimension?
	int passengers; // 0-13, 0-12 having numbers, 13+ being '+'
	CityUnitState state;
	bool operator==(const VehicleTileInfo &other) const;
};
}