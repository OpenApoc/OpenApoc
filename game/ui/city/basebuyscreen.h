#pragma once

#include "framework/stage.h"
#include "library/sp.h"

namespace OpenApoc
{

class Building;
class Base;
class GameState;
class Form;
class Graphic;

class BaseBuyScreen : public Stage
{
  private:
	static const int COST_PER_TILE = 2000;

	sp<Form> form;
	sp<Graphic> baseView;
	int price;

	sp<GameState> state;
	sp<Base> base;
	void renderBase();

  public:
	BaseBuyScreen(sp<GameState> state, sp<Building> building);
	~BaseBuyScreen() override;
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
