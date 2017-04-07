#pragma once

#include "framework/stage.h"
#include "library/sp.h"
#include <functional>

namespace OpenApoc
{
class GameState;
class EventMessage;
class CityView;
class BattleView;
class Form;
class Control;
class FormsEvent;

class MessageLogScreen : public Stage
{
  private:
	sp<Form> menuform;

	sp<GameState> state;

	sp<Control> createMessageRow(EventMessage message, sp<GameState> state, CityView &cityView);
	sp<Control> createMessageRow(EventMessage message, sp<GameState> state, BattleView &battleView);
	sp<Control> createMessageRow(EventMessage message, sp<GameState> state,
	                             std::function<void(FormsEvent *e)> callback);

  public:
	MessageLogScreen(sp<GameState> state, CityView &cityView);
	MessageLogScreen(sp<GameState> state, BattleView &battleView);
	~MessageLogScreen() override;
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
