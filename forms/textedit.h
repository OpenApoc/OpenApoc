
#pragma once

#include "control.h"
#include "forms_enums.h"

#define TEXTEDITOR_CARET_TOGGLE_TIME 30

namespace OpenApoc
{

class BitmapFont;
class Framework;

class TextEdit : public Control
{

  private:
	bool caretDraw;
	int caretTimer;
	UString text;
	std::shared_ptr<BitmapFont> font;
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

	TextEdit(Framework &fw, Control *Owner, UString Text, std::shared_ptr<BitmapFont> font);
	virtual ~TextEdit();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	UString GetText();
	void SetText(UString Text);

	std::shared_ptr<BitmapFont> GetFont();
	void SetFont(std::shared_ptr<BitmapFont> NewFont);
};

}; // namespace OpenApoc
