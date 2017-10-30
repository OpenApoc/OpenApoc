#include "forms/form.h"
#include "game/state/city/vehicle.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/shared/aequipment.h"
#include "library/sp.h"

namespace OpenApoc
{

/*
 * Implements some methods to display the stat sheet for an agent equipment item/itemtype
 * To be used in agent equipment and transaction screens
*/
class AEquipmentSheet
{
  public:
	AEquipmentSheet(sp<Form> form);
	void display(sp<AEquipment> item, bool researched = true);
	void display(sp<AEquipmentType> itemType, bool researched = true);
	void clear();

  private:
	void displayImplementation(sp<AEquipment> item, sp<AEquipmentType> itemType, bool researched);
	void displayGrenade(sp<AEquipment> item, sp<AEquipmentType> itemType);
	void displayWeapon(sp<AEquipment> item, sp<AEquipmentType> itemType);
	void displayAmmo(sp<AEquipment> item, sp<AEquipmentType> itemType);
	void displayArmor(sp<AEquipment> item, sp<AEquipmentType> itemType);
	void displayOther(sp<AEquipment> item, sp<AEquipmentType> itemType);
	void displayAlien(sp<AEquipment> item, sp<AEquipmentType> itemType);
	sp<Form> form;
};

}; // namespace OpenApoc
