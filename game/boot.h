
#pragma once

#include "framework/stage.h"
#include "framework/includes.h"

namespace OpenApoc {

class Image;

class BootUp : public Stage
{
	private:
		std::shared_ptr<Image> loadingimage;
		std::shared_ptr<Image> logoimage;
		int loadtime;
		Angle<float> loadingimageangle;

		void StartGame();

		static void CreateGameCore(Framework &fw);

	public:
		BootUp(Framework &fw) : Stage(fw){};
		// Stage control
		virtual void Begin();
		virtual void Pause();
		virtual void Resume();
		virtual void Finish();
		virtual void EventOccurred(Event *e);
		virtual void Update(StageCmd * const cmd);
		virtual void Render();
		virtual bool IsTransition();
};

}; //namespace OpenApoc
