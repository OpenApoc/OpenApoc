
#pragma once
#include "library/sp.h"

#include "control.h"
#include "forms_enums.h"
#include "framework/font.h"

namespace OpenApoc
{

class Label : public Control
{

  private:
	UString text;
	sp<BitmapFont> font;

  protected:
	void onRender() override;

  public:
	HorizontalAlignment TextHAlign;
	VerticalAlignment TextVAlign;
	bool WordWrap;

	Label(const UString &Text = "", sp<BitmapFont> font = nullptr);
	~Label() override;

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;

	UString getText() const;
	void setText(const UString &Text);

	sp<BitmapFont> getFont() const;
	void setFont(sp<BitmapFont> NewFont);

	sp<Control> copyTo(sp<Control> CopyParent) override;
	void configureSelfFromXml(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
