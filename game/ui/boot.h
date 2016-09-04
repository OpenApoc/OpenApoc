#pragma once
#include "framework/includes.h"
#include "framework/stage.h"
#include "library/sp.h"

namespace OpenApoc
{

class Image;

class BootUp : public Stage
{
  private:
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
