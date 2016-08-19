
#pragma once
#include "library/sp.h"

#include "control.h"
#include "forms_enums.h"

namespace OpenApoc
{

class BitmapFont;
class Sample;
class Image;
class Label;

class TextButton : public Control
{

  private:
	sp<Label> label;
	sp<Surface> cached;

	sp<Sample> buttonclick;
	sp<Image> buttonbackground;

  protected:
	void OnRender() override;

  public:
	enum class ButtonRenderStyle
	{
		Flat,
		Bevel,
		Menu
	};

	HorizontalAlignment TextHAlign;
	VerticalAlignment TextVAlign;
	ButtonRenderStyle RenderStyle;

	TextButton(const UString &Text = "", sp<BitmapFont> font = nullptr);
	~TextButton() override;

	void EventOccured(Event *e) override;
	void Update() override;
	void UnloadResources() override;

	UString GetText() const;
	void SetText(const UString &Text);

	sp<BitmapFont> GetFont() const;
	void SetFont(sp<BitmapFont> NewFont);

	sp<Control> CopyTo(sp<Control> CopyParent) override;
	void ConfigureSelfFromXML(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
