
#pragma once
#include "control.h"
#include "forms_enums.h"
#include "framework/font.h"
#include "library/sp.h"
#include <queue>

namespace OpenApoc
{

class Ticker : public Control
{

  private:
	static const int ANIM_TICKS = 60;
	static const int DISPLAY_TICKS = 600;

	bool animating;
	int animTimer, displayTimer;

	UString text;
	std::queue<UString> messages;
	sp<BitmapFont> font;

  protected:
	void onRender() override;

  public:
	HorizontalAlignment TextHAlign;
	VerticalAlignment TextVAlign;

	Ticker(sp<BitmapFont> font = nullptr);
	~Ticker() override;

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;

	void addMessage(const UString &Text);

	sp<BitmapFont> getFont() const;
	void setFont(sp<BitmapFont> NewFont);

	sp<Control> copyTo(sp<Control> CopyParent) override;
	void configureSelfFromXml(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
