
#pragma once

#include "control.h"

class VScrollBar : public Control
{
	private:
		bool capture;

		void LoadResources();

	protected:
		virtual void OnRender();

	public:
		ALLEGRO_COLOR GripperColour;
		int Minimum;
		int Maximum;
		int Value;
		int LargeChange;

		VScrollBar( Control* Owner );
		virtual ~VScrollBar();

		virtual void EventOccured( Event* e );
		virtual void Update();
		virtual void UnloadResources();
};