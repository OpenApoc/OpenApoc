
#pragma once

#include "control.h"
#include "framework/logger.h"

#include <typeinfo>

namespace OpenApoc
{

class Framework;

class Form : public Control
{

  protected:
	virtual void OnRender() override;

  public:
	Form(Framework &fw, tinyxml2::XMLElement *FormConfiguration);
	virtual ~Form();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	template <typename T> T *FindControlTyped(const UString &name)
	{
		Control *c = this->FindControl(name);
		if (!c)
		{
			LogError("Failed to find control \"%s\" within form \"%s\"", name.str().c_str(),
			         this->Name.str().c_str());
			return nullptr;
		}
		T *typedControl = dynamic_cast<T *>(c);
		if (!c)
		{
			LogError("Failed cast  control \"%s\" within form \"%s\" to type \"%s\"",
			         name.str().c_str(), this->Name.str().c_str(), typeid(T).name());
			return nullptr;
		}
		return typedControl;
	}
};

}; // namespace OpenApoc
