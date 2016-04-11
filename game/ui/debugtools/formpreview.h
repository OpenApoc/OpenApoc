
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
