#pragma once

#include "framework/stage.h"
#include "library/sp.h"

namespace OpenApoc
{

class Form;

class CreditsMenu : public Stage
{
  private:
	sp<Form> menuform;

  public:
	CreditsMenu();
	~CreditsMenu() override;

	void loadlist();

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
} // namespace OpenApoc