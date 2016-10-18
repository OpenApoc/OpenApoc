#pragma once

#include "framework/stage.h"
#include "library/sp.h"
#include "library/strings.h"

namespace OpenApoc
{

class GameState;
class CityView;
class Form;

class NotificationScreen : public Stage
{
  private:
	sp<Form> menuform;

	sp<GameState> state;

  public:
	NotificationScreen(sp<GameState> state, CityView &cityView, const UString &message);
	~NotificationScreen() override;
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
