
#pragma once

#include "framework/includes.h"
#include "framework/event.h"

namespace OpenApoc {

class Form;

class Control
{
	private:
		ALLEGRO_BITMAP* controlArea;

		void PreRender();
		void PostRender();


	protected:
		Control* owningControl;
		Control* focusedChild;
		bool mouseInside;
		bool mouseDepressed;
		Vector2 resolvedLocation;

		virtual void OnRender();

		void SetFocus(Control* Child);
		bool IsFocused();

		void ResolveLocation();
		void ConfigureFromXML( tinyxml2::XMLElement* Element );

		Control* GetRootControl();

	public:
		std::string Name;
		Vector2 Location;
		Vector2 Size;
		ALLEGRO_COLOR BackgroundColour;

		std::vector<Control*> Controls;

		Control(Control* Owner);
		virtual ~Control();

		Control* GetActiveControl();
		void Focus();

		virtual void EventOccured(Event* e);
		void Render();
		virtual void Update();
		virtual void UnloadResources();

		Control* operator[]( int Index );
		Control* FindControl( std::string ID );

		Control* GetParent();
		Form* GetForm();
};

}; //namespace OpenApoc
