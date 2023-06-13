#pragma once

#include "framework/stage.h"
#include "library/sp.h"

namespace OpenApoc
{

class GameState;
class Form;

class InGameOptions : public Stage
{
  private:
	sp<Form> menuform;

	sp<GameState> state;

  public:
	InGameOptions(sp<GameState> state);
	~InGameOptions() override;

	void saveList();
	void loadList();

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
