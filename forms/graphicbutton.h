
#pragma once

#include "control.h"
#include "../game/apocresources/rawsound.h"

class GraphicButton : public Control
{

	private:
		std::string image_name;
		std::string imagedepressed_name;
		std::string imagehover_name;
		ALLEGRO_BITMAP* image;
		ALLEGRO_BITMAP* imagedepressed;
		ALLEGRO_BITMAP* imagehover;

		static RawSound* buttonclick;

	public:
		GraphicButton( Control* Owner, std::string Image, std::string ImageDepressed );
		GraphicButton( Control* Owner, std::string Image, std::string ImageDepressed, std::string ImageHover );
		virtual ~GraphicButton();

		virtual void EventOccured( Event* e );
		virtual void Render();
		virtual void Update();
		virtual void UnloadResources();

		ALLEGRO_BITMAP* GetImage();
		void SetImage( ALLEGRO_BITMAP* Image );
		ALLEGRO_BITMAP* GetDepressedImage();
		void SetDepressedImage( ALLEGRO_BITMAP* Image );
		ALLEGRO_BITMAP* GetHoverImage();
		void SetHoverImage( ALLEGRO_BITMAP* Image );
};

