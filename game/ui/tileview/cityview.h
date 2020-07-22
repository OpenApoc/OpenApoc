#pragma once

#include "game/state/stateobject.h"
#include "game/ui/tileview/citytileview.h"
#include "library/sp.h"
#include <map>
#include <vector>

namespace OpenApoc
{

constexpr int ALIEN_INCIDENT_SCORE = -30;

class Form;
class GameState;
class GraphicButton;
class Control;
class Vehicle;
class Sample;
class Base;
class Building;
class Projectile;
class Organisation;
class VehicleTileInfo;
class Agent;
class UfopaediaEntry;
class AgentInfo;
class OrganisationInfo;
class VEquipmentType;

enum class CityUpdateSpeed
{
	Pause,
	Speed1,
	Speed2,
	Speed3,
	Speed4,
	Speed5,
};

enum class CitySelectionState
{
	Normal,
	GotoBuilding,
	GotoLocation,
	AttackVehicle,
	AttackBuilding,
	ManualControl
};

class CityView : public CityTileView
{
  private:
	sp<Form> activeTab, baseForm;
	std::vector<sp<Form>> uiTabs;
	sp<Form> overlayTab;
	std::vector<sp<GraphicButton>> miniViews;
	CityUpdateSpeed updateSpeed;
	CityUpdateSpeed lastSpeed;

	sp<GameState> state;

	std::vector<VehicleTileInfo> ownedVehicleInfoList;
	std::vector<AgentInfo> ownedSoldierInfoList;
	std::vector<AgentInfo> ownedBioInfoList;
	std::vector<AgentInfo> ownedEngineerInfoList;
	std::vector<AgentInfo> ownedPhysicsInfoList;
	std::vector<VehicleTileInfo> hostileVehicleInfoList;
	std::vector<OrganisationInfo> organisationInfoList;
	std::vector<StateRef<VEquipmentType>> weaponType;
	std::vector<bool> weaponDisabled;
	std::vector<int> weaponAmmo;

	bool followVehicle;

	void updateSelectedUnits();

	CitySelectionState selectionState;
	bool modifierLShift = false;
	bool modifierRShift = false;
	bool modifierLAlt = false;
	bool modifierRAlt = false;
	bool modifierLCtrl = false;
	bool modifierRCtrl = false;

	bool vanillaControls = false;

	bool drawCity = true;
	sp<Surface> surface;

	std::vector<sp<Image>> debugLabelsOK;
	std::vector<sp<Image>> debugLabelsDead;

	// Click handlers

	bool handleClickedBuilding(StateRef<Building> building, bool rightClick,
	                           CitySelectionState selState);
	bool handleClickedVehicle(StateRef<Vehicle> vehicle, bool rightClick,
	                          CitySelectionState selState, bool passThrough = false);
	bool handleClickedAgent(StateRef<Agent> agent, bool rightClick, CitySelectionState selState);
	bool handleClickedProjectile(sp<Projectile> projectile, bool rightClick,
	                             CitySelectionState selState);
	bool handleClickedOrganisation(StateRef<Organisation> organisation, bool rightClick,
	                               CitySelectionState selState);

	void tryOpenUfopaediaEntry(StateRef<UfopaediaEntry> ufopaediaEntry);

	// Orders

	void orderGoToBase();
	void orderMove(Vec3<float> position, bool alternative, bool portal = false);
	void orderMove(StateRef<Building> building, bool alternative);
	void orderSelect(StateRef<Vehicle> vehicle, bool inverse, bool additive);
	void orderSelect(StateRef<Agent> agent, bool inverse, bool additive);
	void orderFire(Vec3<float> position);
	void orderAttack(StateRef<Vehicle> vehicle, bool forced);
	void orderFollow(StateRef<Vehicle> vehicle);
	void orderAttack(StateRef<Building> building);
	void orderDisableWeapon(int index, bool disable);

  public:
	CityView(sp<GameState> state);
	~CityView() override;

	void initiateUfoMission(StateRef<Vehicle> ufo, StateRef<Vehicle> playerCraft);
	void initiateBuildingMission(sp<GameState> state, StateRef<Building> building,
	                             std::list<StateRef<Agent>> agents);

	void begin() override;
	void resume() override;
	void refreshBaseView();
	void update() override;
	void render() override;
	void eventOccurred(Event *e) override;
	bool handleKeyDown(Event *e);
	bool handleKeyUp(Event *e);
	bool handleMouseDown(Event *e);
	bool handleGameStateEvent(Event *e);

	void setUpdateSpeed(CityUpdateSpeed updateSpeed);
	void zoomLastEvent();
	void setSelectionState(CitySelectionState selectionState);
	void setSelectedTab(int tabIndex);
};

}; // namespace OpenApoc
