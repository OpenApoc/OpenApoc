#pragma once

#include "framework/stage.h"
#include "library/sp.h"

namespace OpenApoc
{

class GameState;
class Form;

class UfopaediaView : public Stage
{
  private:
	sp<Form> menuform;
	sp<GameState> state;

  public:
	UfopaediaView(sp<GameState> state);
	~UfopaediaView() override;
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
