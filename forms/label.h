
#pragma once
#include "library/sp.h"

#include "control.h"
#include "framework/font.h"
#include "forms_enums.h"

namespace OpenApoc
{

class Label : public Control
{

  private:
	UString text;
	sp<BitmapFont> font;

  protected:
	virtual void OnRender() override;

  public:
	HorizontalAlignment TextHAlign;
	VerticalAlignment TextVAlign;
	bool WordWrap;

	Label(Control *Owner, UString Text = "", sp<BitmapFont> font = nullptr);
	virtual ~Label();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	UString GetText() const;
	void SetText(UString Text);

	sp<BitmapFont> GetFont() const;
	void SetFont(sp<BitmapFont> NewFont);

	virtual Control *CopyTo(Control *CopyParent) override;
	virtual void ConfigureFromXML(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
