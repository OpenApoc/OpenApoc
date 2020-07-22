#pragma once

#include "forms/control.h"
#include "forms/forms_enums.h"
#include "library/sp.h"
#include "library/strings.h"
#include <queue>

namespace OpenApoc
{

class BitmapFont;

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
	bool hasMessages() const { return !text.empty() || !messages.empty(); }

	sp<BitmapFont> getFont() const;
	void setFont(sp<BitmapFont> NewFont);

	sp<Control> copyTo(sp<Control> CopyParent) override;
	void configureSelfFromXml(pugi::xml_node *node) override;
};

}; // namespace OpenApoc
