#pragma once

#include "forms/control.h"
#include "forms/forms_enums.h"
#include "library/sp.h"
#include "library/strings.h"

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
	void onRender() override;

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

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;

	UString getText() const;
	void setText(const UString &Text);

	sp<BitmapFont> getFont() const;
	void setFont(sp<BitmapFont> NewFont);

	sp<Control> copyTo(sp<Control> CopyParent) override;
	void configureSelfFromXml(pugi::xml_node *node) override;
};

}; // namespace OpenApoc
