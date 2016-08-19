
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
	void OnRender() override;

  public:
	HorizontalAlignment ImageHAlign;
	VerticalAlignment ImageVAlign;
	FillMethod ImagePosition;
	bool AutoSize;

	Graphic(sp<Image> Image = nullptr);
	~Graphic() override;

	void EventOccured(Event *e) override;
	void Update() override;
	void UnloadResources() override;

	sp<Image> GetImage() const;
	void SetImage(sp<Image> Image);

	sp<Control> CopyTo(sp<Control> CopyParent) override;
	void ConfigureSelfFromXML(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
