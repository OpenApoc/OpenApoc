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

		Vec3<float> getCentrePosition();
		sp<VoxelMap> getVoxelMap() override;
		Vec3<float> getPosition() const override;

	private:
		friend class TileMap;
		std::weak_ptr<BattleUnit> unit;
		std::list<sp<Image>>::iterator animationFrame;

		TileObjectBattleUnit(TileMap &map, sp<BattleUnit> unit);
	};

} // namespace OpenApoc
