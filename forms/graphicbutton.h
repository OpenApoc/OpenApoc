
#pragma once

#include "control.h"

namespace OpenApoc {

class Sample;
class Image;

class GraphicButton : public Control
{

	private:
		std::string image_name;
		std::string imagedepressed_name;
		std::string imagehover_name;
		std::shared_ptr<Image> image;
		std::shared_ptr<Image> imagedepressed;
		std::shared_ptr<Image> imagehover;

		std::shared_ptr<Sample> buttonclick;

	protected:
		virtual void OnRender();

	public:
		GraphicButton( Framework &fw, Control* Owner, std::string Image, std::string ImageDepressed );
		GraphicButton( Framework &fw, Control* Owner, std::string Image, std::string ImageDepressed, std::string ImageHover );
		virtual ~GraphicButton();

		virtual void EventOccured( Event* e );
		virtual void Update();
		virtual void UnloadResources();

		std::shared_ptr<Image> GetImage();
		void SetImage( std::shared_ptr<Image> Image );
		std::shared_ptr<Image> GetDepressedImage();
		void SetDepressedImage( std::shared_ptr<Image> Image );
		std::shared_ptr<Image> GetHoverImage();
		void SetHoverImage( std::shared_ptr<Image> Image );
};

}; //namespace OpenApoc
