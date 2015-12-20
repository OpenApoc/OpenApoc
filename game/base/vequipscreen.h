#pragma once

#include "framework/stage.h"
#include "library/sp.h"
#include "game/rules/vequipment.h"

namespace OpenApoc
{

class Vehicle;
class Form;
class Palette;

class VEquipScreen : public Stage
{
  private:
	StageCmd stageCmd;
	up<Form> form;
	sp<Vehicle> selected;
	VEquipmentType::Type selectionType;
	sp<Palette> pal;

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
