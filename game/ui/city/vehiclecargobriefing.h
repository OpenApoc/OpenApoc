#pragma once

#include "framework/stage.h"
#include "library/sp.h"
#include <game/state/city/vehicle.h>

namespace OpenApoc
{

class Form;
class Control;
class FormsEvent;

class VehicleCargoBriefing : public Stage
{
  private:
	sp<Form> menuform;
	std::list<Cargo> vehicleCargo;
	bool asc;

	void refreshListBoxes();
	void sortItems(std::list<Cargo> &list, bool byName = true, bool asc = true);
	void removeZeroCountItems(std::list<Cargo> &list);
	UString convertToText(const UString &item);

  public:
	VehicleCargoBriefing(sp<Vehicle> vehicle);
	~VehicleCargoBriefing() override;

	// Stage control
	void begin() override;
	void pause() override;
	void resume() override;
	void finish() override;
	void eventOccurred(Event *e) override;
	void update() override;
	void render() override;
	bool isTransition() override;
};

}; // namespace OpenApoc
