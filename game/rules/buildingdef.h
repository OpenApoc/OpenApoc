#pragma once

#include "library/rect.h"

namespace OpenApoc
{
	class UString;

	class BuildingDef
	{
	private:
		BuildingDef();
		UString name;
		Rect<int> bounds;
		int ownerIdx;
		friend class RulesLoader;
	public:
		const UString& getName() const {return this->name;}
		const Rect<int>& getBounds() const {return this->bounds;}
		int getOwnerIdx() const {return this->ownerIdx;}
	};
};
