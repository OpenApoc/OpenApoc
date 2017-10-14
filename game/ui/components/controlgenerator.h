#pragma once
#include "game/state/stateobject.h"
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
	sp<Image> purchaseBoxIcon;
	sp<Image> purchaseXComIcon;
	sp<Image> purchaseArrow;
	sp<Image> alienContainedDetain;
	sp<Image> alienContainedKill;
	sp<Image> scrollLeft;
	sp<Image> scrollRight;
	sp<Image> transactionShade;

  public:
	static VehicleTileInfo createVehicleInfo(GameState &state, sp<Vehicle> v);
	static sp<Control> createVehicleControl(GameState &state, const VehicleTileInfo &info);
	static sp<Control> createVehicleControl(GameState &state, sp<Vehicle> v);

	static AgentInfo
	createAgentInfo(GameState &state, sp<Agent> a,
	                UnitSelectionState forcedSelectionState = UnitSelectionState::NA,
	                bool forceFade = false);
	static void fillAgentControl(GameState &state, sp<Graphic> baseControl, const AgentInfo &info);
	static sp<Control> createAgentControl(GameState &state, const AgentInfo &info);
	static sp<Control>
	createAgentControl(GameState &state, sp<Agent> a,
	                   UnitSelectionState forcedSelectionState = UnitSelectionState::NA,
	                   bool forceFade = false);
	static sp<Control> createLargeAgentControl(GameState &state, const AgentInfo &info,
	                                           bool addSkill = false, bool labMode = false);
	static sp<Control>
	createLargeAgentControl(GameState &state, sp<Agent> a, bool addSkill = false,
	                        UnitSelectionState forcedSelectionState = UnitSelectionState::NA,
	                        bool forceFade = false, bool labMode = false);

	static OrganisationInfo createOrganisationInfo(GameState &state, sp<Organisation> org);
	static sp<Control> createOrganisationControl(GameState &state, const OrganisationInfo &info);
	static sp<Control> createOrganisationControl(GameState &state, sp<Organisation> org);

	// Buying/selling agent equipment and ammo
	// Also used for sacking alien containment
	static sp<Control>
	createPurchaseControl(GameState &state, StateRef<AEquipmentType> agentEquipmentType, int stock);
	// Buying/selling vehicle equipment
	static sp<Control> createPurchaseControl(GameState &state,
	                                         StateRef<VEquipmentType> vehicleEquipmentType,
	                                         int stock);
	// Buying/selling vehicle ammo and fuel
	static sp<Control> createPurchaseControl(GameState &state, StateRef<VAmmoType> vehicleAmmoType,
	                                         int stock);
	// Buying vehicles
	static sp<Control> createPurchaseControl(GameState &state, StateRef<VehicleType> vehicleType,
	                                         int stock);
	// Selling vehicles
	static sp<Control> createPurchaseControl(GameState &state, StateRef<Vehicle> vehicle,
	                                         int stock);

	// Transferring agent equipment and ammo
	// Also used for transfer of alien bodies
	static sp<Control> createTransferControl(GameState &state,
	                                         StateRef<AEquipmentType> agentEquipmentType,
	                                         int stock1, int stock2);
	// Transferring vehicle equipment
	static sp<Control> createTransferControl(GameState &state,
	                                         StateRef<VEquipmentType> vehicleEquipmentType,
	                                         int stock, int stock2);
	// Transferring vehicle ammo
	static sp<Control> createTransferControl(GameState &state, StateRef<VAmmoType> vehicleAmmoType,
	                                         int stock, int stock2);
	// Transferring vehicles
	static sp<Control> createTransferControl(GameState &state, StateRef<Vehicle> vehicleType,
	                                         int stock, int stock2);

	// Creates the control
	static sp<Control> createTransactionControl(GameState &state, bool isAmmo, sp<Image> iconLeft,
	                                            sp<Image> iconRight, bool transfer, UString name,
	                                            UString manufacturer, int price, int stock1,
	                                            int stock2);

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
}