#pragma once
#include "forms/forms_enums.h"
#include "framework/font.h"
#include "framework/logger.h"
#include "library/colour.h"
#include "library/sp.h"
#include <functional>
#include <list>
#include <map>

namespace tinyxml2
{
class XMLElement;
} // namespace tinyxml2

namespace OpenApoc
{

class Form;
class Event;
class Surface;
class Palette;
class RadioButton;
class Control;
class RadioButtonGroup;
class FormsEvent;

class Control : public std::enable_shared_from_this<Control>
{
  private:
	sp<Surface> controlArea;
	sp<void> data;

	std::map<FormEventType, std::list<std::function<void(FormsEvent *e)>>> callbacks;

  protected:
	sp<Palette> palette;
	wp<Control> owningControl;
	bool mouseInside;
	bool mouseDepressed;
	Vec2<int> resolvedLocation;

	virtual void PreRender();
	virtual void PostRender();
	virtual void OnRender();

	virtual bool IsFocused() const;

	void ResolveLocation();
	virtual void ConfigureFromXML(tinyxml2::XMLElement *Element);

	sp<Control> GetRootControl();

	void CopyControlData(sp<Control> CopyOf);

	void pushFormEvent(FormEventType type, Event *parentEvent);

	void triggerEventCallbacks(FormsEvent *e);

  public:
	UString Name;
	Vec2<int> Location;
	Vec2<int> Size;
	Colour BackgroundColour;
	bool takesFocus;
	bool showBounds;
	bool Visible;

	bool canCopy;
	wp<Control> lastCopiedTo;
	std::vector<sp<Control>> Controls;

	std::map<UString, sp<RadioButtonGroup>> radiogroups;

	Control(bool takesFocus = true);
	virtual ~Control();

	sp<Control> GetActiveControl() const;

	virtual void EventOccured(Event *e);
	void Render();
	virtual void Update();
	virtual void UnloadResources();

	sp<Control> operator[](int Index) const;
	sp<Control> FindControl(UString ID) const;

	template <typename T> sp<T> FindControlTyped(const UString &name) const
	{
		auto c = this->FindControl(name);
		if (!c)
		{
			LogError("Failed to find control \"%s\" within form \"%s\"", name.c_str(),
			         this->Name.c_str());
			return nullptr;
		}
		sp<T> typedControl = std::dynamic_pointer_cast<T>(c);
		if (!typedControl)
		{
			LogError("Failed to cast control \"%s\" within form \"%s\" to type \"%s\"",
			         name.c_str(), this->Name.c_str(), typeid(T).name());
			return nullptr;
		}
		return typedControl;
	}

	sp<Control> GetParent() const;
	sp<Form> GetForm();
	void SetParent(sp<Control> Parent);
	sp<Control> GetAncestor(sp<Control> Parent);

	Vec2<int> GetLocationOnScreen() const;

	virtual sp<Control> CopyTo(sp<Control> CopyParent);

	template <typename T> sp<T> GetData() const { return std::static_pointer_cast<T>(data); }
	void SetData(sp<void> Data) { data = Data; }

	bool eventIsWithin(const Event *e) const;

	template <typename T, typename... Args> sp<T> createChild(Args &&... args)
	{
		sp<T> newControl = mksp<T>(std::forward<Args>(args)...);
		newControl->SetParent(shared_from_this());
		return newControl;
	}

	void addCallback(FormEventType event, std::function<void(FormsEvent *e)> callback);
};

}; // namespace OpenApoc
