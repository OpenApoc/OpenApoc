#pragma once

#include "forms/listbox.h"
#include "framework/stage.h"
#include "library/sp.h"

namespace OpenApoc
{

class GameState;
class Form;

class MoreOptions : public Stage
{
  private:
	sp<Form> menuform;
	sp<GameState> state;

	UString getOptionFullName(const UString &optionSection, const UString &optionName) const;

	bool GetIfOptionInt(const UString &optionFullName) const;
	bool GetIfOptionInt(const UString &optionSection, const UString &optionName) const;

	bool GetIfOptionFloat(const UString &optionFullName) const;
	bool GetIfOptionFloat(const UString &optionSection, const UString &optionName) const;

	void configureOptionControlAndAddToControlListBox(const sp<Control> &control,
	                                                  const UString &optionSection,
	                                                  const UString &optionName,
	                                                  const sp<BitmapFont> &font,
	                                                  const sp<ListBox> &listControl);

	void addChildLabelToControl(const sp<Control> &control, const UString &optionSection,
	                            const UString &optionName, const sp<BitmapFont> &font,
	                            const sp<ListBox> &listControl);

  public:
	MoreOptions(sp<GameState> state);
	~MoreOptions() override;

	void saveLists();
	void loadLists();

	// Stage control
	void begin() override;
	void pause() override;
	void resume() override;
	void finish() override;
	void eventOccurred(Event *e) override;
	void update() override;
	void render() override;
	bool isTransition() override;
};
} // namespace OpenApoc