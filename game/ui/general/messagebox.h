
#pragma once

#include "framework/stage.h"

#include "forms/forms.h"

#include <functional>

namespace OpenApoc
{

class MessageBox : public Stage
{
  private:
	StageCmd stageCmd;
	sp<Form> form;
	std::function<void()> callbackYes;
	std::function<void()> callbackNo;

  public:
	enum class ButtonOptions
	{
		Ok,
		YesNo
	};

	MessageBox(const UString &title, const UString &text, ButtonOptions buttons,
	           std::function<void()> callbackYes = std::function<void()>(),
	           std::function<void()> callbackNo = std::function<void()>());
	~MessageBox() override;
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
