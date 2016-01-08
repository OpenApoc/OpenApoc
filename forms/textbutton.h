
#pragma once
#include "library/sp.h"

#include "control.h"
#include "forms_enums.h"

namespace OpenApoc
{

class BitmapFont;
class Sample;
class Image;

class TextButton : public Control
{

  private:
	UString text;
	sp<BitmapFont> font;
	sp<Surface> cached;

	sp<Sample> buttonclick;
	sp<Image> buttonbackground;

  protected:
	virtual void OnRender() override;

  public:
	enum class TextButtonRenderStyles
	{
		SolidButtonStyle,
		MenuButtonStyle
	};

	HorizontalAlignment TextHAlign;
	VerticalAlignment TextVAlign;
	TextButtonRenderStyles RenderStyle;

	TextButton(Control *Owner, UString Text, sp<BitmapFont> font);
	virtual ~TextButton();

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
