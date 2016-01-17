
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
	sp<Image> image;
	sp<Image> imagedepressed;
	sp<Image> imagehover;

	sp<Sample> buttonclick;

  protected:
	virtual void OnRender() override;

  public:
	sp<ScrollBar> ScrollBarPrev, ScrollBarNext;

	GraphicButton(sp<Image> image = nullptr, sp<Image> imageDepressed = nullptr,
	              sp<Image> imageHover = nullptr);
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

	virtual sp<Control> CopyTo(sp<Control> CopyParent) override;
	virtual void ConfigureFromXML(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
