
#pragma once

#include "framework/stage.h"

#include "forms/forms.h"

namespace OpenApoc
{

class MessageBox : public Stage
{
  private:
	StageCmd stageCmd;
	sp<Form> form;

  public:
	enum class ButtonOptions
	{
		Ok,
		YesNo
	};

	MessageBox(const UString &title, const UString &text, ButtonOptions buttons);
	~MessageBox();
	// Stage control
	virtual void Begin() override;
	virtual void Pause() override;
	virtual void Resume() override;
	virtual void Finish() override;
	virtual void EventOccurred(Event *e) override;
	virtual void Update(StageCmd *const cmd) override;
	virtual void Render() override;
	virtual bool IsTransition() override;
};
}; // namespace OpenApoc
