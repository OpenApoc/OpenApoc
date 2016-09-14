#pragma once

#include "framework/stage.h"
#include "library/sp.h"
#include <map>

namespace OpenApoc
{

class Form;
class GameState;
class Palette;

class AEquipScreen : public Stage
{
  private:
	sp<Form> form;
	sp<Palette> pal;
	sp<GameState> state;

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
};

} // namespace OpenApoc
