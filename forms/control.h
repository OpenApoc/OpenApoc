#pragma once

#include "forms/forms_enums.h"
#include "framework/logger.h"
#include "library/colour.h"
#include "library/sp.h"
#include "library/vec.h"
#include <functional>
#include <list>
#include <map>

namespace pugi
{
class xml_node;
} // namespace pugi

namespace OpenApoc
{

class BitmapFont;
class Form;
class Event;
class Surface;
class Palette;
class Control;
class RadioButtonGroup;
class FormsEvent;

class Control : public std::enable_shared_from_this<Control>
{
  private:
	sp<Surface> controlArea;
	sp<void> data;

	std::map<FormEventType, std::list<std::function<void(FormsEvent *e)>>> callbacks;

	// Configures children of element after it was configured, see ConfigureFromXML
	void configureChildrenFromXml(pugi::xml_node *parent);
	// The function will be called during pre-rendering of the control.
	// arg - this control
	std::function<void(sp<Control>)> funcPreRender;

	bool dirty = true;

  protected:
	sp<Palette> palette;
	wp<Control> owningControl;
	bool mouseInside;
	bool mouseDepressed;
	Vec2<int> resolvedLocation;

	virtual void postRender();
	virtual void onRender();

	virtual bool isFocused() const;

	void resolveLocation();
	bool isPointInsideControlBounds(int x, int y) const;

	// Loads control and all subcontrols from xml
	void configureFromXml(pugi::xml_node *node);
	// configures current element from xml element (without children)
	virtual void configureSelfFromXml(pugi::xml_node *node);

	sp<Control> getRootControl();

	void pushFormEvent(FormEventType type, Event *parentEvent);

	void triggerEventCallbacks(FormsEvent *e);

	bool isDirty() const { return dirty; }

	bool Visible;
	bool isClickable;

  public:
	UString Name;
	// Relative location.
	Vec2<int> Location;
	// Size of the control.
	Vec2<int> Size;
	// Which area size will be selected.
	Vec2<int> SelectionSize;
	Colour BackgroundColour;
	bool takesFocus;
	bool showBounds;
	bool Enabled;
	bool canCopy;
	wp<Control> lastCopiedTo;
	std::vector<sp<Control>> Controls;

	UString ToolTipText;
	sp<BitmapFont> ToolTipFont;
	// transparent background by default
	Colour ToolTipBackground;
	std::vector<std::pair<unsigned int, Colour>> ToolTipBorders;

	std::map<UString, sp<RadioButtonGroup>> radiogroups;

	void copyControlData(sp<Control> CopyOf);

	Control(bool takesFocus = true);
	virtual ~Control();

	virtual void eventOccured(Event *e);
	// Used if controls require computations before rendering.
	void preRender();
	void render();
	virtual void update();
	virtual void unloadResources();

	sp<Control> operator[](int Index) const;
	sp<Control> findControl(UString ID) const;
	bool replaceChildByName(sp<Control> ctrl);

	void setDirty();
	void setVisible(bool value);
	bool isVisible() const { return Visible; };

	template <typename T> sp<T> findControlTyped(const UString &name) const
	{
		auto c = this->findControl(name);
		if (!c)
		{
			LogError("Failed to find control \"%s\" within form \"%s\"", name, this->Name);
			return nullptr;
		}
		sp<T> typedControl = std::dynamic_pointer_cast<T>(c);
		if (!typedControl)
		{
			LogError("Failed to cast control \"%s\" within form \"%s\" to type \"%s\"", name,
			         this->Name, typeid(T).name());
			return nullptr;
		}
		return typedControl;
	}

	sp<Control> getParent() const;
	sp<Form> getForm();
	void setParent(sp<Control> Parent, int position);
	void setParent(sp<Control> Parent);
	sp<Control> getAncestor(sp<Control> Parent);

	Vec2<int> getLocationOnScreen() const { return resolvedLocation; }

	void setRelativeWidth(float widthPercent);
	void setRelativeHeight(float widthPercent);

	Vec2<int> getParentSize() const;
	static int align(HorizontalAlignment HAlign, int ParentWidth, int ChildWidth);
	static int align(VerticalAlignment VAlign, int ParentHeight, int ChildHeight);

	void align(HorizontalAlignment HAlign);
	void align(VerticalAlignment VAlign);
	void align(HorizontalAlignment HAlign, VerticalAlignment VAlign);

	virtual sp<Control> copyTo(sp<Control> CopyParent);

	template <typename T> sp<T> getData() const { return std::static_pointer_cast<T>(data); }
	void setData(sp<void> Data) { data = Data; }

	bool eventIsWithin(const Event *e) const;
	bool isPointInsideControlBounds(Event *e, sp<Control> c) const;

	template <typename T, typename... Args> sp<T> createChild(Args &&... args)
	{
		sp<T> newControl = mksp<T>(std::forward<Args>(args)...);
		newControl->setParent(shared_from_this());
		return newControl;
	}

	void addCallback(FormEventType event, std::function<void(FormsEvent *e)> callback);

	// Simulate mouse click on control
	virtual bool click();
	// Setter for funcPreRender
	void setFuncPreRender(std::function<void(sp<Control>)> func) { funcPreRender = func; }
};

}; // namespace OpenApoc
