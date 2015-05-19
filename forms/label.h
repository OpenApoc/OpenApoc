
#pragma once

#include "control.h"
#include "framework/font.h"
#include "forms_enums.h"

namespace OpenApoc {

class Framework;

class Label : public Control
{

	private:
		UString text;
		std::shared_ptr<BitmapFont> font;

	protected:
		virtual void OnRender();

	public:
		HorizontalAlignment TextHAlign;
		VerticalAlignment TextVAlign;

		Label(Framework &fw, Control* Owner, UString Text, std::shared_ptr<BitmapFont> font);
		virtual ~Label();

		virtual void EventOccured(Event* e);
		virtual void Update();
		virtual void UnloadResources();

		UString GetText();
		void SetText( UString Text );
};

}; //namespace OpenApoc
