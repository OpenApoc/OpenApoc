
#pragma once

#include "control.h"

class Form : public Control
{

	public:
		Form( tinyxml2::XMLElement* FormConfiguration );
		~Form();

		virtual void EventOccured( Event* e );
		virtual void Render();
		virtual void Update();
		virtual void UnloadResources();
};

