
#pragma once

#include "control.h"
#include "forms_enums.h"

namespace OpenApoc {

class BitmapFont;
class Sample;
class Framework;
class Image;

class TextButton : public Control
{

	private:
		UString text;
		std::shared_ptr<BitmapFont> font;
		std::shared_ptr<Surface> cached;

		std::shared_ptr<Sample> buttonclick;
		std::shared_ptr<Image> buttonbackground;

	protected:
		virtual void OnRender();

	public:
		HorizontalAlignment TextHAlign;
		VerticalAlignment TextVAlign;

		TextButton(Framework &fw, Control* Owner, UString Text, std::shared_ptr<BitmapFont> font);
		virtual ~TextButton();

		virtual void EventOccured(Event* e);
		virtual void Update();
		virtual void UnloadResources();

		UString GetText();
		void SetText( UString Text );

		std::shared_ptr<BitmapFont> GetFont();
		void SetFont(std::shared_ptr<BitmapFont> NewFont);
};

}; //namespace OpenApoc
