#pragma once

#include "control.h"

namespace OpenApoc
{

class Form : public Control
{

  protected:
	void onRender() override;

  public:
	Form(tinyxml2::XMLElement *FormConfiguration);
	Form();
	~Form() override;

	virtual void readFormStyle(tinyxml2::XMLElement *FormConfiguration);

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;

	sp<Control> copyTo(sp<Control> CopyParent) override;
};

}; // namespace OpenApoc
