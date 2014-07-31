
#pragma once

#include "control.h"
#include "../game/apocresources/rawsound.h"

class CheckBox : public Control
{

	private:
		ALLEGRO_BITMAP* imagechecked;
		ALLEGRO_BITMAP* imageunchecked;

		static RawSound* buttonclick;

		void LoadResources();

	public:
		bool Checked;

		CheckBox( Control* Owner );
		~CheckBox();

		virtual void EventOccured( Event* e );
		virtual void Render();
		virtual void Update();
		virtual void UnloadResources();
};
