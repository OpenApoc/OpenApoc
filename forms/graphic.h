
#pragma once
#include "library/sp.h"

#include "control.h"
#include "forms_enums.h"
#include "framework/image.h"

namespace OpenApoc
{

class Graphic : public Control
{

  private:
	sp<Image> image;

  protected:
	void onRender() override;

  public:
	HorizontalAlignment ImageHAlign;
	VerticalAlignment ImageVAlign;
	FillMethod ImagePosition;
	bool AutoSize;

	Graphic(sp<Image> Image = nullptr);
	~Graphic() override;

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;

	sp<Image> getImage() const;
	void setImage(sp<Image> Image);

	sp<Control> copyTo(sp<Control> CopyParent) override;
	void configureSelfFromXml(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
