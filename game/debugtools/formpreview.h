
#pragma once

#include "framework/stage.h"

#include "forms/forms.h"

namespace OpenApoc
{

class FormPreview : public Stage
{
  private:
	sp<CheckBox> interactWithDisplay;
	sp<Label> currentSelected;
	StageCmd stageCmd;
	sp<Form> previewselector;
	sp<Form> propertyeditor;
	sp<Form> displayform;

	int glowindex;

	sp<Control> currentSelectedControl;

	void ConfigureSelectedControlForm();

  public:
	FormPreview();
	~FormPreview();
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
