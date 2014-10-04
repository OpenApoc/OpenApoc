
#pragma once

#include "control.h"
#include "game/resources/ifont.h"
#include "game/apocresources/rawsound.h"

namespace OpenApoc {

class TextButton : public Control
{

	private:
		std::string text;
		IFont* font;

		static RawSound* buttonclick;
		std::shared_ptr<Image> buttonbackground;

	protected:
		virtual void OnRender();

	public:
		HorizontalAlignment TextHAlign;
		VerticalAlignment TextVAlign;

		TextButton(Control* Owner, std::string Text, IFont* Font);
		virtual ~TextButton();

		virtual void EventOccured(Event* e);
		virtual void Update();
		virtual void UnloadResources();

		std::string GetText();
		void SetText( std::string Text );
};

}; //namespace OpenApoc
