
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
	sp<Form> previewselector;
	sp<Form> propertyeditor;
	sp<Form> displayform;
	sp<TextButton> reloadButton;

	int glowindex;

	sp<Control> currentSelectedControl;

	void configureSelectedControlForm();

  public:
	FormPreview();
	~FormPreview() override;
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
