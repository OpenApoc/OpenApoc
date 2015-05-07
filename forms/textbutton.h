
#pragma once

#include "control.h"
#include "forms_enums.h"

namespace OpenApoc {

class IFont;
class Sample;
class Framework;
class Image;

class TextButton : public Control
{

	private:
		std::string text;
		IFont* font;
		std::shared_ptr<Surface> cached;

		std::shared_ptr<Sample> buttonclick;
		std::shared_ptr<Image> buttonbackground;

	protected:
		virtual void OnRender();

	public:
		HorizontalAlignment TextHAlign;
		VerticalAlignment TextVAlign;

		TextButton(Framework &fw, Control* Owner, std::string Text, IFont* Font);
		virtual ~TextButton();

		virtual void EventOccured(Event* e);
		virtual void Update();
		virtual void UnloadResources();

		std::string GetText();
		void SetText( std::string Text );
};

}; //namespace OpenApoc
