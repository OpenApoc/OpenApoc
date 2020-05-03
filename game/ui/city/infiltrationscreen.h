#pragma once

#include "framework/stage.h"
#include "library/sp.h"
#include <array>
#include <optional>

namespace OpenApoc
{

class Form;
class GameState;
class Organisation;
class Label;
class Graphic;

class InfiltrationScreen : public Stage
{
  private:
	sp<Form> menuform;
	sp<Graphic> graph;

	sp<GameState> state;
	std::array<Label *, 10> shown_org_names;
	std::array<const Organisation *, 10> shown_orgs;
	void reset_shown_orgs();
	void update_view();

  public:
	InfiltrationScreen(sp<GameState> state);
	~InfiltrationScreen() override;
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
