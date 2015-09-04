#pragma once

#include "framework/stage.h"
#include "framework/includes.h"
#include "ufopaediaentry.h"

namespace OpenApoc
{

class UfopaediaCategory : public Stage // , public std::enable_shared_from_this<UfopaediaCategory>
{
  private:
	Form *menuform;
	StageCmd stageCmd;

	void SetCatOffset(int Direction);

  public:
	UString ID;
	UString Title;
	UString BodyInformation;
	UString BackgroundImageFilename;
	std::vector<std::shared_ptr<UfopaediaEntry>> Entries;

	int ViewingEntry;

	UfopaediaCategory(Framework &fw, tinyxml2::XMLElement *Element);
	~UfopaediaCategory();

	// Stage control
	virtual void Begin() override;
	virtual void Pause() override;
	virtual void Resume() override;
	virtual void Finish() override;
	virtual void EventOccurred(Event *e) override;
	virtual void Update(StageCmd *const cmd) override;
	virtual void Render() override;
	virtual bool IsTransition() override;

	void SetTopic(int Index);
	void SetupForm();

	void SetPrevCat();
	void SetNextCat();
};
} // namespace OpenApoc
