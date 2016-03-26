#pragma once
#include "library/sp.h"

#include "framework/includes.h"
#include "framework/stage.h"
#include "game/ufopaedia.h"

namespace OpenApoc
{

class Form;
class GameState;

class UfopaediaCategoryView : public Stage
{
  private:
	sp<Form> menuform;
	StageCmd stageCmd;
	GameState &state;
	sp<UfopaediaCategory> category;

	// The iterator showing the current position of the entry within the category.
	// When equal to category->entries.end() it will show the category description.
	std::map<UString, sp<UfopaediaEntry>>::iterator position_iterator;

	void setFormData();

  public:
	UfopaediaCategoryView(GameState &state, sp<UfopaediaCategory> cat);
	~UfopaediaCategoryView();

	// Stage control
	virtual void Begin() override;
	virtual void Pause() override;
	virtual void Resume() override;
	virtual void Finish() override;
	virtual void EventOccurred(Event *e) override;
	virtual void Update(StageCmd *const cmd) override;
	virtual void Render() override;
	virtual bool IsTransition() override;
};
}; // namespace OpenApoc
