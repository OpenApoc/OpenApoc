
#pragma once

#include "framework/stage.h"
#include "framework/includes.h"

#include "game/resources/gamecore.h"
#include "game/apocresources/apocresource.h"
#include "forms/forms.h"

namespace OpenApoc
{

class FormPreview : public Stage
{
  private:
	CheckBox *interactWithDisplay;
	Label *currentSelected;
	StageCmd stageCmd;
	std::unique_ptr<Form> previewselector;
	std::unique_ptr<Form> propertyeditor;
	std::unique_ptr<Form> displayform;

	int glowindex;

	Control *currentSelectedControl;

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
