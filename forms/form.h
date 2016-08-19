#pragma once

#include "control.h"

namespace OpenApoc
{

class Form : public Control
{

  protected:
	void OnRender() override;

  public:
	Form(tinyxml2::XMLElement *FormConfiguration);
	Form();
	~Form() override;

	virtual void ReadFormStyle(tinyxml2::XMLElement *FormConfiguration);

	void EventOccured(Event *e) override;
	void Update() override;
	void UnloadResources() override;

	sp<Control> CopyTo(sp<Control> CopyParent) override;
};

}; // namespace OpenApoc
