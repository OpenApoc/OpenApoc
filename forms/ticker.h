
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
	void OnRender() override;

  public:
	HorizontalAlignment TextHAlign;
	VerticalAlignment TextVAlign;

	Ticker(sp<BitmapFont> font = nullptr);
	~Ticker() override;

	void EventOccured(Event *e) override;
	void Update() override;
	void UnloadResources() override;

	void AddMessage(const UString &Text);

	sp<BitmapFont> GetFont() const;
	void SetFont(sp<BitmapFont> NewFont);

	sp<Control> CopyTo(sp<Control> CopyParent) override;
	void ConfigureSelfFromXML(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
