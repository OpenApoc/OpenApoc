
#pragma once

#include "control.h"

namespace OpenApoc {

class Framework;

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

		VScrollBar( Framework &fw, Control* Owner );
		virtual ~VScrollBar();

		virtual void EventOccured( Event* e );
		virtual void Update();
		virtual void UnloadResources();
};

}; //namespace OpenApoc
