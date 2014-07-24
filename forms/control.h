
#pragma once

#include "../framework/includes.h"
#include "../library/vector2.h"
#include "../framework/event.h"

class Control
{

	private:
		Control* owningControl;
		Control* focusedChild;
		bool mouseInside;
		bool mouseDepressed;
		
	protected:
		void PreRender();
		void PostRender();
		void SetFocus(Control* Child);
		bool IsFocused();

	public:
		std::vector<Control*> Controls;
		Vector2 Location;
		Vector2 Size;
		ALLEGRO_COLOR BackgroundColour;

		Control(Control* Owner);
		virtual ~Control();

		Vector2* GetResolvedLocation();
		Control* GetActiveControl();
		void Focus();

		virtual void EventOccured(Event* e, bool* WasHandled) = 0;
		virtual void Render();
		virtual void Update() = 0;

};

