
#pragma once

#include "framework/stage.h"

#include "forms/forms.h"

namespace OpenApoc
{

class ImagePreview : public Stage
{
  private:
	sp<Form> menuform;
	StageCmd stageCmd;
	sp<TextEdit> imageFilename;
	sp<Graphic> imageView;

	void updateImage();

  public:
	ImagePreview();
	~ImagePreview() override;
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
