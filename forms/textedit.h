
#pragma once
#include "library/sp.h"

#include "control.h"
#include "forms_enums.h"

#define TEXTEDITOR_CARET_TOGGLE_TIME 30

namespace OpenApoc
{

class BitmapFont;

class TextEdit : public Control
{

  private:
	bool caretDraw;
	int caretTimer;
	UString text;
	sp<BitmapFont> font;
	bool editting;
	bool editShift;
	bool editAltGr;

	void RaiseEvent(FormEventType Type);

  protected:
	virtual void OnRender() override;

  public:
	unsigned int SelectionStart;
	HorizontalAlignment TextHAlign;
	VerticalAlignment TextVAlign;

	TextEdit(Control *Owner, UString Text, sp<BitmapFont> font);
	virtual ~TextEdit();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	UString GetText();
	void SetText(UString Text);

	sp<BitmapFont> GetFont();
	void SetFont(sp<BitmapFont> NewFont);

	virtual Control *CopyTo(Control *CopyParent) override;
};

}; // namespace OpenApoc
