
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
	void OnRender() override;

  public:
	HorizontalAlignment TextHAlign;
	VerticalAlignment TextVAlign;
	bool WordWrap;

	Label(const UString &Text = "", sp<BitmapFont> font = nullptr);
	~Label() override;

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
