
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
	void onRender() override;

  public:
	sp<ScrollBar> ScrollBarPrev, ScrollBarNext;

	GraphicButton(sp<Image> image = nullptr, sp<Image> imageDepressed = nullptr,
	              sp<Image> imageHover = nullptr);
	~GraphicButton() override;

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;

	sp<Image> getImage() const;
	void setImage(sp<Image> Image);
	sp<Image> getDepressedImage() const;
	void setDepressedImage(sp<Image> Image);
	sp<Image> getHoverImage() const;
	void setHoverImage(sp<Image> Image);

	sp<Control> copyTo(sp<Control> CopyParent) override;
	void configureSelfFromXml(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
