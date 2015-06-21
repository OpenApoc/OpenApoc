
#pragma once

#include "control.h"

namespace OpenApoc {

class HScrollBar : public Control
{
	private:
		bool capture;

		void LoadResources();

	protected:
		virtual void OnRender();

	public:
		Colour GripperColour;
		int Minimum;
		int Maximum;
		int Value;
		int LargeChange;

		HScrollBar( Framework &fw, Control* Owner );
		virtual ~HScrollBar();

		virtual void EventOccured( Event* e );
		virtual void Update();
		virtual void UnloadResources();
		virtual void SetValue(int newValue);
};

}; //namespace OpenApoc
