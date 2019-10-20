#pragma once

#include "framework/stage.h"
#include "library/sp.h"

namespace OpenApoc
{

class Form;

class DifficultyMenu : public Stage
{
  private:
	sp<Form> difficultymenuform;

  public:
	DifficultyMenu();
	~DifficultyMenu() override;
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
