#pragma once

#include "framework/stage.h"
#include "framework/includes.h"
#include "ufopaediaentry.h"

namespace OpenApoc {

class UfopaediaCategory : public Stage // , public std::enable_shared_from_this<UfopaediaCategory>
{
	private:
		Form* menuform;
		StageCmd stageCmd;

		void SetCatOffset(int Direction);

	public:
		UString ID;
		UString Title;
		UString BodyInformation;
		UString BackgroundImageFilename;
		std::vector<std::shared_ptr<UfopaediaEntry>> Entries;

		int ViewingEntry;

		UfopaediaCategory(Framework &fw, tinyxml2::XMLElement* Element);
		~UfopaediaCategory();

		// Stage control
		virtual void Begin();
		virtual void Pause();
		virtual void Resume();
		virtual void Finish();
		virtual void EventOccurred(Event *e);
		virtual void Update(StageCmd * const cmd);
		virtual void Render();
		virtual bool IsTransition();

		void SetTopic(int Index);
		void SetupForm();

		void SetPrevCat();
		void SetNextCat();
};
}; //namespace OpenApoc
