
#pragma once
#include "library/sp.h"

#include "control.h"
#include "forms_enums.h"

#define TEXTEDITOR_CARET_TOGGLE_TIME 5

namespace OpenApoc
{

class BitmapFont;

class TextEdit : public Control
{

  private:
	bool caretDraw;
	int caretTimer;
	UString text;
	UString cursor;
	sp<BitmapFont> font;
	bool editting;
	bool editShift;
	bool editAltGr;
	UString allowedCharacters;
	size_t textMaxLength = std::string::npos;
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
	void SetCursor(const UString &cursor);
	void SetTextMaxSize(size_t length);
	// set to empty string to allow everything
	void SetAllowedCharacters(const UString &allowedCharacters);

	sp<BitmapFont> GetFont() const;
	void SetFont(sp<BitmapFont> NewFont);

	sp<Control> CopyTo(sp<Control> CopyParent) override;
	void ConfigureSelfFromXML(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
