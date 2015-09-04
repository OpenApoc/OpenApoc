
#pragma once

#include "framework/image.h"
#include "control.h"

namespace OpenApoc
{

class Graphic : public Control
{

  private:
	UString image_name;
	std::shared_ptr<Image> image;

  protected:
	virtual void OnRender() override;

  public:
	Graphic(Framework &fw, Control *Owner, UString Image);
	virtual ~Graphic();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	std::shared_ptr<Image> GetImage();
	void SetImage(std::shared_ptr<Image> Image);
};

} // namespace OpenApoc
