#pragma once
#include "framework/stage.h"
#include "game/state/ufopaedia.h"
#include "library/sp.h"

namespace OpenApoc
{

class Form;
class GameState;
class Label;

class UfopaediaCategoryView : public Stage
{
  private:
	sp<Form> menuform;
	StageCmd stageCmd;
	sp<GameState> state;
	sp<UfopaediaCategory> category;
	std::vector<sp<Label>> orgLabels, orgValues;
	std::vector<sp<Label>> statsLabels, statsValues;
	int baseY;

	// The iterator showing the current position of the entry within the category.
	// When equal to category->entries.end() it will show the category description.
	std::map<UString, sp<UfopaediaEntry>>::iterator position_iterator;

	void setFormData();
	void setFormStats();

  public:
	UfopaediaCategoryView(sp<GameState> state, sp<UfopaediaCategory> cat,
	                      sp<UfopaediaEntry> entry = nullptr);
	~UfopaediaCategoryView();

	// Stage control
	void Begin() override;
	void Pause() override;
	void Resume() override;
	void Finish() override;
	void EventOccurred(Event *e) override;
	void Update(StageCmd *const cmd) override;
	void Render() override;
	bool IsTransition() override;
};
}; // namespace OpenApoc
