
#pragma once
#include "library/sp.h"

#include "control.h"

namespace OpenApoc
{

class Sample;
class Image;
class ScrollBar;

class GraphicButton : public Control
{

  private:
	UString image_name;
	UString imagedepressed_name;
	UString imagehover_name;
	sp<Image> image;
	sp<Image> imagedepressed;
	sp<Image> imagehover;

	sp<Sample> buttonclick;

  protected:
	virtual void OnRender() override;

  public:
	ScrollBar *ScrollBarPrev, *ScrollBarNext;

	GraphicButton(Control *Owner, UString Image, UString ImageDepressed);
	GraphicButton(Control *Owner, UString Image, UString ImageDepressed, UString ImageHover);
	virtual ~GraphicButton();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	sp<Image> GetImage() const;
	void SetImage(sp<Image> Image);
	sp<Image> GetDepressedImage() const;
	void SetDepressedImage(sp<Image> Image);
	sp<Image> GetHoverImage() const;
	void SetHoverImage(sp<Image> Image);

	virtual Control *CopyTo(Control *CopyParent) override;
};

}; // namespace OpenApoc
