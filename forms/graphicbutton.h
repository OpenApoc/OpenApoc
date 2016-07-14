
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
	void OnRender() override;

  public:
	sp<ScrollBar> ScrollBarPrev, ScrollBarNext;

	GraphicButton(sp<Image> image = nullptr, sp<Image> imageDepressed = nullptr,
	              sp<Image> imageHover = nullptr);
	virtual ~GraphicButton();

	void EventOccured(Event *e) override;
	void Update() override;
	void UnloadResources() override;

	sp<Image> GetImage() const;
	void SetImage(sp<Image> Image);
	sp<Image> GetDepressedImage() const;
	void SetDepressedImage(sp<Image> Image);
	sp<Image> GetHoverImage() const;
	void SetHoverImage(sp<Image> Image);

	sp<Control> CopyTo(sp<Control> CopyParent) override;
	void ConfigureSelfFromXML(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
