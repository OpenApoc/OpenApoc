
#pragma once
#include "library/sp.h"

#include "framework/image.h"
#include "control.h"

namespace OpenApoc
{

class Graphic : public Control
{

  private:
	UString image_name;
	sp<Image> image;

  protected:
	virtual void OnRender() override;

  public:
	Graphic(Framework &fw, Control *Owner, UString Image);
	virtual ~Graphic();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	sp<Image> GetImage();
	void SetImage(sp<Image> Image);

	virtual Control *CopyTo(Control *CopyParent) override;
};

}; // namespace OpenApoc
