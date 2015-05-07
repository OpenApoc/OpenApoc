
#pragma once

#include "control.h"

namespace OpenApoc {

class Sample;
class Image;

class CheckBox : public Control
{

	private:
		std::shared_ptr<Image> imagechecked;
		std::shared_ptr<Image> imageunchecked;

		std::shared_ptr<Sample> buttonclick;

		void LoadResources();

	protected:
		virtual void OnRender();

	public:
		bool Checked;

		CheckBox( Framework &fw, Control* Owner );
		virtual ~CheckBox();

		virtual void EventOccured( Event* e );
		virtual void Update();
		virtual void UnloadResources();
};

}; //namespace OpenApoc
