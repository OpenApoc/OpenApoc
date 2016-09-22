#pragma once

#include "game/state/battle/battleunit.h"
#include "game/state/battle/battlestrategyiconlist.h"
#include "game/state/tileview/tileobject.h"

namespace OpenApoc
{

	class TileObjectBattleUnit : public TileObject
	{
	public:
		void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
			TileViewMode mode, int currentLevel) override;
		~TileObjectBattleUnit() override;

		sp<BattleUnit> getUnit() const;

		// For aiming at the object
		Vec3<float> getCentrePosition();
		Vec3<float> getCenterOffset() const override { return{ 0.0f, 0.0f, bounds_div_2.z }; }
		Vec3<float> getVoxelOffset() const override { return{ bounds_div_2.x, bounds_div_2.y, 0.0f }; }
		sp<VoxelMap> getVoxelMap() override;
		Vec3<float> getPosition() const override;

	private:
		friend class TileMap;
		std::weak_ptr<BattleUnit> unit;
		std::list<sp<Image>>::iterator animationFrame;

		TileObjectBattleUnit(TileMap &map, sp<BattleUnit> unit);
	};

} // namespace OpenApoc
