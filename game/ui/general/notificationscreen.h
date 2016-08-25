#pragma once

#include "framework/stage.h"

#include "forms/forms.h"

namespace OpenApoc
{

class GameState;
class CityView;

class NotificationScreen : public Stage
{
  private:
	sp<Form> menuform;
	StageCmd stageCmd;

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
	void update(StageCmd *const cmd) override;
	void render() override;
	bool isTransition() override;
};
}; // namespace OpenApoc
