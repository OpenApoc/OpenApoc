#pragma once

#include <vector>

#include "control.h"

namespace OpenApoc
{

class Form : public Control
{
  public:
	// Every Form currently constructed, in creation order. Registered here because there is no
	// other way for a test harness to find the controls that are actually on screen -- with a
	// resizable viewport and UI scaling, computing rects from the .form XML is not reliable.
	static const std::vector<Form *> &liveForms();

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
