#pragma once

#include "framework/stage.h"

namespace OpenApoc
{

class BootUp : public Stage
{
  public:
	BootUp() : Stage() {}
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
