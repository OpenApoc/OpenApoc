
#pragma once

#include "control.h"
#include "forms_enums.h"

#define TEXTEDITOR_CARET_TOGGLE_TIME		30

namespace OpenApoc {

class BitmapFont;
class Framework;

class TextEdit : public Control
{

	private:
		bool caretDraw;
		int caretTimer;
		std::string text;
		std::shared_ptr<BitmapFont> font;
		bool editting;
		bool editShift;
		bool editAltGr;

		void RaiseEvent( FormEventType Type );

	protected:
		virtual void OnRender();

	public:
		unsigned int SelectionStart;
		HorizontalAlignment TextHAlign;
		VerticalAlignment TextVAlign;

		TextEdit(Framework &fw, Control* Owner, std::string Text, std::shared_ptr<BitmapFont> font);
		virtual ~TextEdit();

		virtual void EventOccured(Event* e);
		virtual void Update();
		virtual void UnloadResources();

		std::string GetText();
		void SetText( std::string Text );
};

}; //namespace OpenApoc
