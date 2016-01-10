
#pragma once
#include "library/sp.h"

#include "framework/includes.h"
#include "library/colour.h"
#include "framework/font.h"
#include "framework/logger.h"

namespace OpenApoc
{

class Form;
class Event;
class Surface;
class Palette;

class Control
{
  private:
	sp<Surface> controlArea;

  protected:
	sp<Palette> palette;
	Control *owningControl;
	Control *focusedChild;
	bool mouseInside;
	bool mouseDepressed;
	Vec2<int> resolvedLocation;

	virtual void PreRender();
	virtual void PostRender();
	virtual void OnRender();

	void SetFocus(Control *Child);
	bool IsFocused() const;

	void ResolveLocation();
	virtual void ConfigureFromXML(tinyxml2::XMLElement *Element);

	Control *GetRootControl();

	std::list<UString> WordWrapText(sp<OpenApoc::BitmapFont> UseFont, UString WrapText) const;

	void CopyControlData(Control *CopyOf);

  public:
	UString Name;
	Vec2<int> Location;
	Vec2<int> Size;
	Colour BackgroundColour;
	bool takesFocus;
	bool showBounds;
	bool Visible;

	bool canCopy;
	Control *lastCopiedTo;
	std::vector<Control *> Controls;

	void *Data;

	Control(Control *Owner, bool takesFocus = true);
	virtual ~Control();

	Control *GetActiveControl() const;
	void Focus();

	virtual void EventOccured(Event *e);
	void Render();
	virtual void Update();
	virtual void UnloadResources();

	Control *operator[](int Index) const;
	Control *FindControl(UString ID) const;

	template <typename T> T *FindControlTyped(const UString &name) const
	{
		Control *c = this->FindControl(name);
		if (!c)
		{
			LogError("Failed to find control \"%s\" within form \"%s\"", name.c_str(),
			         this->Name.c_str());
			return nullptr;
		}
		T *typedControl = dynamic_cast<T *>(c);
		if (!c)
		{
			LogError("Failed cast  control \"%s\" within form \"%s\" to type \"%s\"", name.c_str(),
			         this->Name.c_str(), typeid(T).name());
			return nullptr;
		}
		return typedControl;
	}

	Control *GetParent() const;
	Form *GetForm();
	void SetParent(Control *Parent);

	Vec2<int> GetLocationOnScreen() const;

	virtual Control *CopyTo(Control *CopyParent);
};

}; // namespace OpenApoc
