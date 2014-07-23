
#pragma once

#include "../../framework/framework.h"
#include "../../library/vector2.h"


class Control
{

	private:
		Control* owningControl;

	public:
		std::vector<Control*> Controls;
		Vector2 Location;
		Vector2 Size;

		Control(Control* Owner);
		~Control();

		virtual void EventOccured(Event* e) = 0;
		virtual void Render() = 0;
		virtual void Update() = 0;

};

