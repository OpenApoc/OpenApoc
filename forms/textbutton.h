
#pragma once

#include "control.h"
#include "forms_enums.h"

namespace OpenApoc
{

class BitmapFont;
class Sample;
class Framework;
class Image;

class TextButton : public Control
{

  private:
	UString text;
	std::shared_ptr<BitmapFont> font;
	std::shared_ptr<Surface> cached;

	std::shared_ptr<Sample> buttonclick;
	std::shared_ptr<Image> buttonbackground;

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

	TextButton(Framework &fw, Control *Owner, UString Text, std::shared_ptr<BitmapFont> font);
	virtual ~TextButton();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	UString GetText();
	void SetText(UString Text);

	std::shared_ptr<BitmapFont> GetFont();
	void SetFont(std::shared_ptr<BitmapFont> NewFont);

	virtual Control *CopyTo(Control *CopyParent) override;
};

}; // namespace OpenApoc
