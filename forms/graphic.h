
#pragma once

#include "control.h"

class Graphic : public Control
{

	private:
		std::string image_name;
		ALLEGRO_BITMAP* image;

	public:
		Graphic( Control* Owner, std::string Image );
		~Graphic();

		virtual void EventOccured( Event* e );
		virtual void Render();
		virtual void Update();
		virtual void UnloadResources();

		ALLEGRO_BITMAP* GetImage();
		void SetImage( ALLEGRO_BITMAP* Image );
};

