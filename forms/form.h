
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
	virtual ~Form();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	virtual Control *CopyTo(Control *CopyParent) override;
};

}; // namespace OpenApoc
