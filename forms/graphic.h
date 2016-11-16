#pragma once

#include "forms/control.h"
#include "forms/forms_enums.h"
#include "library/sp.h"

namespace OpenApoc
{

class Image;

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
	void configureSelfFromXml(pugi::xml_node *node) override;
};

}; // namespace OpenApoc
