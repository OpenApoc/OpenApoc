
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
	void OnRender() override;

	bool IsFocused() const override;

  public:
	unsigned int SelectionStart;
	HorizontalAlignment TextHAlign;
	VerticalAlignment TextVAlign;

	TextEdit(const UString &Text = "", sp<BitmapFont> font = nullptr);
	virtual ~TextEdit();

	void EventOccured(Event *e) override;
	void Update() override;
	void UnloadResources() override;

	UString GetText() const;
	void SetText(const UString &Text);

	sp<BitmapFont> GetFont() const;
	void SetFont(sp<BitmapFont> NewFont);

	sp<Control> CopyTo(sp<Control> CopyParent) override;
	void ConfigureFromXML(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
