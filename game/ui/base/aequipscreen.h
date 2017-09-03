#pragma once

#include "game/state/stateobject.h"
#include "framework/stage.h"
#include "library/sp.h"
#include "library/vec.h"
#include <map>

namespace OpenApoc
{

class Form;
class GameState;
class Palette;
class Agent;
class EquipmentPaperDoll;
class Control;
class Image;

class AEquipScreen : public Stage
{
  private:
	sp<Form> form;
	sp<Palette> pal;
	sp<GameState> state;
	sp<Agent> currentAgent;

	sp<EquipmentPaperDoll> paperDoll;

	static const Vec2<int> EQUIP_GRID_SLOT_SIZE;
	static const Vec2<int> EQUIP_GRID_SLOTS;

	sp<Control> createAgentControl(Vec2<int> size, StateRef<Agent> agent);
	// FIXME: healthImage has a copy in CityView - maybe opportunity to merge?
	sp<Image> healthImage;
	sp<Image> shieldImage;
	sp<Image> stunImage;
	sp<Image> iconShade;
	std::vector<sp<Image>> unitRanks;
	std::vector<sp<Image>> bigUnitRanks;

  public:
	AEquipScreen(sp<GameState> state);
	~AEquipScreen() override;

	void begin() override;
	void pause() override;
	void resume() override;
	void finish() override;
	void eventOccurred(Event *e) override;
	void update() override;
	void render() override;
	bool isTransition() override;

	void setSelectedAgent(sp<Agent> agent);
};

} // namespace OpenApoc
