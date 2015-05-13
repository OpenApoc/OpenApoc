
#pragma once

#include "control.h"
#include "framework/font.h"
#include "forms_enums.h"

namespace OpenApoc {

class Framework;

class Label : public Control
{

	private:
		std::string text;
		std::shared_ptr<BitmapFont> font;

	protected:
		virtual void OnRender();

	public:
		HorizontalAlignment TextHAlign;
		VerticalAlignment TextVAlign;

		Label(Framework &fw, Control* Owner, std::string Text, std::shared_ptr<BitmapFont> font);
		virtual ~Label();

		virtual void EventOccured(Event* e);
		virtual void Update();
		virtual void UnloadResources();

		std::string GetText();
		void SetText( std::string Text );
};

}; //namespace OpenApoc
