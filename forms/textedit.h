
#pragma once

#include "control.h"
#include "../game/resources/ifont.h"

#define TEXTEDITOR_CARET_TOGGLE_TIME		30

class TextEdit : public Control
{

	private:
		bool caretDraw;
		int caretTimer;
		std::string text;
		IFont* font;
		bool editting;
		bool editShift;
		bool editAltGr;

	protected:
		virtual void OnRender();

	public:
		int SelectionStart;
		//int SelectionLength;
		HorizontalAlignment TextHAlign;
		VerticalAlignment TextVAlign;

		TextEdit(Control* Owner, std::string Text, IFont* Font);
		virtual ~TextEdit();

		virtual void EventOccured(Event* e);
		virtual void Update();
		virtual void UnloadResources();

		std::string GetText();
		void SetText( std::string Text );
};

