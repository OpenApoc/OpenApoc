
#pragma once
#include "library/sp.h"

#include "control.h"
#include "framework/font.h"
#include "forms_enums.h"

namespace OpenApoc
{

class Framework;

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

	Label(Framework &fw, Control *Owner, UString Text, sp<BitmapFont> font);
	virtual ~Label();

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
