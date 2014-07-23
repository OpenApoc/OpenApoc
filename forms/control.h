
#pragma once

#include "../framework/includes.h"
#include "../library/vector2.h"
#include "../framework/event.h"

class Control
{

	private:
		Control* owningControl;

	protected:
		void PreRender();
		void PostRender();

	public:
		std::vector<Control*> Controls;
		Vector2 Location;
		Vector2 Size;
		ALLEGRO_COLOR BackgroundColour;

		Control(Control* Owner);
		virtual ~Control();

		Vector2* GetResolvedLocation();

		virtual void EventOccured(Event* e, bool* WasHandled) = 0;
		virtual void Render();
		virtual void Update() = 0;

};

