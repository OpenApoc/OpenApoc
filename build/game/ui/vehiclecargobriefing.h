#pragma once

#include <string>
#include <vector>
#include <ranges>
#include <sstream>
#include "framework/stage.h"
#include "library/sp.h"
#include <functional>
#include <game/state/city/vehicle.h>
#include <game/state/message.h>

namespace OpenApoc
{
class Form;
class Control;
class FormsEvent;
class EventMessage;

class VehicleCargoBriefing : public Stage
{
  private:
	bool asc;
	std::list<Cargo> _vehicleCargo;
	sp<Form> _menuform;

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
