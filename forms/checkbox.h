
#pragma once

#include "control.h"
#include "../game/apocresources/rawsound.h"

class CheckBox : public Control
{

	private:
		std::shared_ptr<Image> imagechecked;
		std::shared_ptr<Image> imageunchecked;

		static RawSound* buttonclick;

		void LoadResources();

	protected:
		virtual void OnRender();

	public:
		bool Checked;

		CheckBox( Control* Owner );
		virtual ~CheckBox();

		virtual void EventOccured( Event* e );
		virtual void Update();
		virtual void UnloadResources();
};
