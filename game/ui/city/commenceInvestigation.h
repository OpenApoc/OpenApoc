#pragma once

#include "framework/stage.h"
#include "library/sp.h"

namespace OpenApoc
{
class GameState;
class Building;

class CommenceInvestigation : public Stage
{
  private:
	sp<GameState> state;
	sp<Building> building;

  public:
	CommenceInvestigation();
	~CommenceInvestigation() override;

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
