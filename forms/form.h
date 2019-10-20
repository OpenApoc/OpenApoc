#pragma once

#include "control.h"

namespace OpenApoc
{

class Form : public Control
{

  protected:
	void onRender() override;

  public:
	Form(pugi::xml_node *node);
	Form();
	~Form() override;

	virtual void readFormStyle(pugi::xml_node *node);

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;

	sp<Control> copyTo(sp<Control> CopyParent) override;

	static sp<Form> loadForm(const UString &path);
};

}; // namespace OpenApoc
