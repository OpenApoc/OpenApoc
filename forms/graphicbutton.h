
#pragma once

#include "control.h"

namespace OpenApoc
{

class Sample;
class Image;

class GraphicButton : public Control
{

  private:
	UString image_name;
	UString imagedepressed_name;
	UString imagehover_name;
	std::shared_ptr<Image> image;
	std::shared_ptr<Image> imagedepressed;
	std::shared_ptr<Image> imagehover;

	std::shared_ptr<Sample> buttonclick;

  protected:
	virtual void OnRender() override;

  public:
	GraphicButton(Framework &fw, Control *Owner, UString Image, UString ImageDepressed);
	GraphicButton(Framework &fw, Control *Owner, UString Image, UString ImageDepressed,
	              UString ImageHover);
	virtual ~GraphicButton();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	std::shared_ptr<Image> GetImage();
	void SetImage(std::shared_ptr<Image> Image);
	std::shared_ptr<Image> GetDepressedImage();
	void SetDepressedImage(std::shared_ptr<Image> Image);
	std::shared_ptr<Image> GetHoverImage();
	void SetHoverImage(std::shared_ptr<Image> Image);
};

} // namespace OpenApoc
