#pragma once

#include "forms/control.h"
#include "library/sp.h"

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
	sp<ScrollBar> ScrollBarPrev, ScrollBarNext, ScrollBarPrevHorizontal, ScrollBarNextHorizontal;

	GraphicButton(sp<Image> image = nullptr, sp<Image> imageDepressed = nullptr,
	              sp<Image> imageHover = nullptr);
	~GraphicButton() override;

	bool click() override;

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;

	sp<Sample> getClickSound() const;
	void setClickSound(sp<Sample> sample);
	sp<Image> getImage() const;
	void setImage(sp<Image> Image);
	sp<Image> getDepressedImage() const;
	void setDepressedImage(sp<Image> Image);
	sp<Image> getHoverImage() const;
	void setHoverImage(sp<Image> Image);

	sp<Control> copyTo(sp<Control> CopyParent) override;
	void configureSelfFromXml(pugi::xml_node *node) override;
};

}; // namespace OpenApoc
