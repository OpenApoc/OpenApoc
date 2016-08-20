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
	~UfopaediaCategoryView() override;

	// Stage control
	void begin() override;
	void pause() override;
	void resume() override;
	void finish() override;
	void eventOccurred(Event *e) override;
	void update(StageCmd *const cmd) override;
	void render() override;
	bool isTransition() override;
};
}; // namespace OpenApoc
