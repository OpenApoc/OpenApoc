#pragma once

#include "control.h"

namespace OpenApoc
{

class Form : public Control
{

  protected:
	virtual void OnRender() override;

  public:
	Form(tinyxml2::XMLElement *FormConfiguration);
	Form();
	virtual ~Form();

	virtual void ReadFormStyle(tinyxml2::XMLElement *FormConfiguration);

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	virtual sp<Control> CopyTo(sp<Control> CopyParent) override;
};

}; // namespace OpenApoc
