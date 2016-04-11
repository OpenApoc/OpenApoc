
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
	~MessageBox();
	// Stage control
	void Begin() override;
	void Pause() override;
	void Resume() override;
	void Finish() override;
	void EventOccurred(Event *e) override;
	void Update(StageCmd *const cmd) override;
	void Render() override;
	bool IsTransition() override;
};
}; // namespace OpenApoc
