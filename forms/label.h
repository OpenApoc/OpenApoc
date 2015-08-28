
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
		virtual void OnRender() override;

	public:
		HorizontalAlignment TextHAlign;
		VerticalAlignment TextVAlign;
		bool WordWrap;

		Label(Framework &fw, Control* Owner, UString Text, std::shared_ptr<BitmapFont> font);
		virtual ~Label();

		virtual void EventOccured(Event* e) override;
		virtual void Update() override;
		virtual void UnloadResources() override;

		UString GetText();
		void SetText( UString Text );

		std::shared_ptr<BitmapFont> GetFont();
		void SetFont(std::shared_ptr<BitmapFont> NewFont);
};

}; //namespace OpenApoc
