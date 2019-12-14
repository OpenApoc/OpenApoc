#pragma once
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <vector>

namespace OpenApoc
{

class Vehicle;
class Agent;
class Building;
class GameState;
class VehicleTileInfo;
class AgentInfo;
class Image;
class BitmapFont;
class Control;
class Facility;
class Graphic;
class AEquipmentType;
class VehicleType;
class VEquipmentType;
class VAmmoType;
class EconomyInfo;
class OrganisationInfo;
class Organisation;

enum class Rank;

enum class CityUnitState
{
	InBase = 0,
	InVehicle = 1,
	InBuilding = 2,
	InMotion = 3,
};

enum class UnitSelectionState
{
	NA = -1,
	Unselected = 0,
	Selected = 1,
	FirstSelected = 2
};

enum class UnitSkillState
{
	Hidden = 0,
	Vertical = 1,
	Horizontal = 2
};

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
	sp<Image> iconFatal;
	sp<Image> iconPsiIn;
	sp<Image> iconPsiOut;
	std::vector<sp<Image>> unitRanks;
	std::vector<sp<Image>> battleSelect;
	std::vector<sp<Image>> citySelect;
	std::vector<sp<Image>> vehiclePassengerCountIcons;
	std::vector<sp<Image>> icons;
	std::vector<sp<Image>> purchaseControlParts;

  public:
	static const UString VEHICLE_ICON_NAME;
	static const UString AGENT_ICON_NAME;
	static const UString LEFT_LIST_NAME;
	static const UString RIGHT_LIST_NAME;

	// Icon of vehicle
	static sp<Control> createVehicleIcon(GameState &state, sp<Vehicle> vehicle);

	static VehicleTileInfo createVehicleInfo(GameState &state, sp<Vehicle> v);
	static sp<Control> createVehicleControl(GameState &state, const VehicleTileInfo &info);
	static sp<Control> createVehicleControl(GameState &state, sp<Vehicle> v);
	// Vehicle control with name
	static sp<Control> createVehicleAssignmentControl(GameState &state, sp<Vehicle> vehicle);
	// Building control for assignment
	static sp<Control> createBuildingAssignmentControl(GameState &state, sp<Building> building);
	// Agent control for assignment state
	static sp<Control> createAgentAssignmentControl(GameState &state, sp<Agent> agent);
	// Icon of agent
	static sp<Control>
	createAgentIcon(GameState &state, sp<Agent> agent,
	                UnitSelectionState forcedSelectionState = UnitSelectionState::NA,
	                bool forceFade = false);

	static AgentInfo
	createAgentInfo(GameState &state, sp<Agent> a,
	                UnitSelectionState forcedSelectionState = UnitSelectionState::NA,
	                bool forceFade = false);
	static CityUnitState getCityUnitState(sp<Agent> agent);
	static void fillAgentControl(GameState &state, sp<Graphic> baseControl, const AgentInfo &info);
	static sp<Control> createAgentControl(GameState &state, const AgentInfo &info);
	static sp<Control>
	createAgentControl(GameState &state, sp<Agent> a,
	                   UnitSelectionState forcedSelectionState = UnitSelectionState::NA,
	                   bool forceFade = false);
	static sp<Control> createLargeAgentControl(GameState &state, const AgentInfo &info, int width,
	                                           UnitSkillState skill = UnitSkillState::Hidden);
	static sp<Control> createLargeAgentControl(
	    GameState &state, sp<Agent> a, int width, UnitSkillState skill = UnitSkillState::Hidden,
	    UnitSelectionState forcedSelectionState = UnitSelectionState::NA, bool forceFade = false);
	// Create lab icon control with quantity label.
	static sp<Control> createLabControl(sp<GameState> state, sp<Facility> facility);
	// Control containing two MultilistBox for assignment state
	static sp<Control> createDoubleListControl(const int controlLength);

	static OrganisationInfo createOrganisationInfo(GameState &state, sp<Organisation> org);
	static sp<Control> createOrganisationControl(GameState &state, const OrganisationInfo &info);
	static sp<Control> createOrganisationControl(GameState &state, sp<Organisation> org);

	static int getFontHeight(GameState &state);
};

// All the info required to draw a single vehicle info chunk, kept together to make it easier to
// track when something has changed and requires a re-draw
class VehicleTileInfo
{
  public:
	sp<Vehicle> vehicle;
	UnitSelectionState selected;
	float healthProportion;
	bool shield;
	bool faded;     // Faded when they enter the alien dimension?
	int passengers; // 0-13, 0-12 having numbers, 13+ being '+'
	CityUnitState state;
	bool operator==(const VehicleTileInfo &other) const;
	bool operator!=(const VehicleTileInfo &other) const;
};

class AgentInfo
{
  public:
	sp<Agent> agent;
	Rank rank;
	bool useRank;
	CityUnitState state;
	bool useState;
	UnitSelectionState selected;

	float healthProportion;
	float stunProportion;
	bool shield;
	bool faded; // Faded when stunned or lowmorale

	bool psiOut;
	bool psiIn;
	bool fatal;

	bool operator==(const AgentInfo &other) const;
	bool operator!=(const AgentInfo &other) const;
};

class OrganisationInfo
{
  public:
	sp<Organisation> organisation;
	bool selected;
	bool operator==(const OrganisationInfo &other) const;
	bool operator!=(const OrganisationInfo &other) const;
};
} // namespace OpenApoc
