#pragma once
#include "game/state/organisation.h"
#include "game/state/stateobject.h"
#include "library/strings.h"
#include "library/vec.h"
#include <map>
#include <set>

namespace OpenApoc
{
	class Rules;
	class Image;
	class Sample;
	class AEquipmentType : public StateObject<AEquipmentType>
	{
	public:
		AEquipmentType();
		
		virtual ~AEquipmentType() = default;

		UString id;
		UString name;

		sp<Image> equipscreen_sprite;
		Vec2<int> equipscreen_size;
		
		int store_space;
		StateRef<Organisation> manufacturer;
	};

} // namespace OpenApoc
