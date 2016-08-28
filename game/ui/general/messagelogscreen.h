#pragma once

#include "framework/stage.h"

#include "forms/forms.h"

namespace OpenApoc
{
class GameState;
class EventMessage;
class CityView;

class MessageLogScreen : public Stage
{
  private:
	sp<Form> menuform;
	StageCmd stageCmd;

	sp<GameState> state;

	sp<Control> createMessageRow(EventMessage message, sp<GameState> state, CityView &cityView);

  public:
	MessageLogScreen(sp<GameState> state, CityView &cityView);
	~MessageLogScreen() override;
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
