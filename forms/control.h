
#pragma once

#include "framework/includes.h"
#include "library/colour.h"
#include "framework/font.h"

namespace OpenApoc
{

class Form;
class Event;
class Framework;
class Surface;

class Control
{
  private:
	std::shared_ptr<Surface> controlArea;

	void PreRender();
	void PostRender();

  protected:
	Control *owningControl;
	Control *focusedChild;
	bool mouseInside;
	bool mouseDepressed;
	Vec2<int> resolvedLocation;

	virtual void OnRender();

	void SetFocus(Control *Child);
	bool IsFocused();

	void ResolveLocation();
	void ConfigureFromXML(tinyxml2::XMLElement *Element);

	Control *GetRootControl();

	std::list<UString> WordWrapText(std::shared_ptr<OpenApoc::BitmapFont> UseFont,
	                                UString WrapText);

	Framework &fw;

  public:
	UString Name;
	Vec2<int> Location;
	Vec2<int> Size;
	Colour BackgroundColour;
	bool takesFocus;
	bool showBounds;
	bool Visible;

	std::vector<Control *> Controls;

	Control(Framework &fw, Control *Owner, bool takesFocus = true);
	virtual ~Control();

	Control *GetActiveControl();
	void Focus();

	virtual void EventOccured(Event *e);
	void Render();
	virtual void Update();
	virtual void UnloadResources();

	Control *operator[](int Index);
	Control *FindControl(UString ID);

	Control *GetParent();
	Form *GetForm();
	void SetParent(Control *Parent);
};

}; // namespace OpenApoc
