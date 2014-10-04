
#pragma once

#include "control.h"
#include "../game/resources/ifont.h"

namespace OpenApoc {

class Label : public Control
{

	private:
		std::string text;
		IFont* font;

	protected:
		virtual void OnRender();

	public:
		HorizontalAlignment TextHAlign;
		VerticalAlignment TextVAlign;

		Label(Control* Owner, std::string Text, IFont* Font);
		virtual ~Label();

		virtual void EventOccured(Event* e);
		virtual void Update();
		virtual void UnloadResources();

		std::string GetText();
		void SetText( std::string Text );
};

}; //namespace OpenApoc
