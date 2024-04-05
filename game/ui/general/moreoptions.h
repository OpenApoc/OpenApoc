#pragma once

#include "forms/listbox.h"
#include "forms/textedit.h"
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

	bool getIfOptionInt(const UString &optionFullName) const;
	bool getIfOptionInt(const UString &optionSection, const UString &optionName) const;

	bool getIfOptionFloat(const UString &optionFullName) const;
	bool getIfOptionFloat(const UString &optionSection, const UString &optionName) const;

	void configureOptionControlAndAddToControlListBox(const sp<Control> &control,
	                                                  const UString &optionSection,
	                                                  const UString &optionName,
	                                                  const sp<ListBox> &listControl,
	                                                  const int &labelLocationHeight);

	void addChildLabelToControl(const sp<Control> &control, const UString &optionSection,
	                            const UString &optionName, const sp<ListBox> &listControl,
	                            const int &labelLocationHeight);

	void addFocusControlCallbackToNumberTextEdit(const std::list<sp<TextEdit>> &textEditList);

	sp<TextEdit> createTextEditForNumericOptions(const UString &optionSection,
	                                             const UString &optionName,
	                                             const sp<ListBox> &listControl,
	                                             const UString &labelText) const;

	void
	addButtonsToNumericOption(const sp<Control> &control, const sp<ListBox> &listControl,
	                          const std::function<void(FormsEvent *e)> &buttonUpClickCallback,
	                          const std::function<void(FormsEvent *e)> &buttonDownClickCallback);

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