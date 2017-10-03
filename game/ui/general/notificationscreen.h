#pragma once

#include "framework/stage.h"
#include "library/sp.h"
#include "library/strings.h"

namespace OpenApoc
{

class GameState;
class CityView;
class BattleView;
class Form;
enum class GameEventType;

class NotificationScreen : public Stage
{
  private:
	sp<Form> menuform;
	GameEventType eventType;
	sp<GameState> state;

  public:
	NotificationScreen(sp<GameState> state, CityView &cityView, const UString &message,
	                   GameEventType eventType);
	NotificationScreen(sp<GameState> state, BattleView &battleView, const UString &message,
	                   GameEventType eventType);
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
