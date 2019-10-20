#pragma once

#include "forms/control.h"
#include "forms/forms_enums.h"
#include "library/sp.h"
#include "library/strings.h"

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
	bool editing;
	UString allowedCharacters;
	size_t textMaxLength = std::string::npos;
	void raiseEvent(FormEventType Type);

  protected:
	void onRender() override;

  public:
	bool isFocused() const override;
	unsigned int SelectionStart;
	HorizontalAlignment TextHAlign;
	VerticalAlignment TextVAlign;

	TextEdit(const UString &Text = "", sp<BitmapFont> font = nullptr);
	~TextEdit() override;

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;

	UString getText() const;
	void setText(const UString &Text);
	void setCursor(const UString &cursor);
	void setTextMaxSize(size_t length);
	// set to empty string to allow everything
	void setAllowedCharacters(const UString &allowedCharacters);

	sp<BitmapFont> getFont() const;
	void setFont(sp<BitmapFont> NewFont);

	sp<Control> copyTo(sp<Control> CopyParent) override;
	void configureSelfFromXml(pugi::xml_node *node) override;
};

}; // namespace OpenApoc
