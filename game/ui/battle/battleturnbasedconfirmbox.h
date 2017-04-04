#pragma once

#include "framework/stage.h"
#include "library/sp.h"
#include "library/strings.h"
#include <functional>

namespace OpenApoc
{

class Form;

class BattleTurnBasedConfirmBox : public Stage
{
  private:
	sp<Form> form;
	std::function<void()> callbackYes;
	std::function<void()> callbackNo;

  public:
	enum class ButtonOptions
	{
		Ok,
		YesNo
	};

	BattleTurnBasedConfirmBox(const UString &text,
	                          std::function<void()> callbackYes = std::function<void()>(),
	                          std::function<void()> callbackNo = std::function<void()>());
	~BattleTurnBasedConfirmBox() override;
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
