#pragma once

#include "framework/stage.h"
#include "library/sp.h"
#include "game/rules/vequipment.h"

namespace OpenApoc
{

class Vehicle;
class Form;
class Palette;
class VEquipment;

class VEquipScreen : public Stage
{
  private:
	StageCmd stageCmd;
	up<Form> form;
	sp<Vehicle> selected;
	VEquipmentType::Type selectionType;
	sp<Palette> pal;

	sp<Vehicle> highlightedVehicle;
	sp<VEquipment> highlightedEquipment;

	sp<VEquipment> draggedEquipment;

	static const Vec2<int> EQUIP_GRID_SLOT_SIZE;
	static const Vec2<int> EQUIP_GRID_SLOTS;

  public:
	VEquipScreen(Framework &fw);
	virtual ~VEquipScreen();

	void Begin() override;
	void Pause() override;
	void Resume() override;
	void Finish() override;
	void EventOccurred(Event *e) override;
	void Update(StageCmd *const cmd) override;
	void Render() override;
	bool IsTransition() override;

	void setSelectedVehicle(sp<Vehicle> vehicle);
};

} // namespace OpenApoc
