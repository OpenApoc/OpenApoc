#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/baselayout.h"
#include "library/strings_format.h"
#include "tools/extractors/common/ufo2p.h"
#include "tools/extractors/extractors.h"

namespace OpenApoc
{

void InitialGameStateExtractor::extractBaseLayouts(GameState &state) const
{
	auto &data = this->ufo2p;
	for (unsigned i = 0; i < data.baselayouts->count(); i++)
	{
		UString id = fmt::format("{}{}", BaseLayout::getPrefix(), i);
		auto layout = mksp<BaseLayout>();
		auto b = data.baselayouts->get(i);
		bool foundLift = false;
		for (int row = 0; row < 8; row++)
		{
			for (int col = 0; col < 8; col++)
			{
				switch (b.module[row][col])
				{
					case 0:
						// 0 == ground
						break;
					case 1:
						// 1 == corridor
						{
							layout->baseCorridors.emplace(col, row, col + 1, row + 1);
							break;
						}
					case 2:
					{
						if (foundLift)
						{
							LogError("Unexpected repeated lift at position {{{},{}}} in base {}",
							         row, col, id);
						}
						foundLift = true;
						layout->baseLift = {col, row};
						// We see 'corridors' as the base bounds, and the access
						// lift should be within that, so mark that as a 'corridor'
						// with an access lift built on top
						layout->baseCorridors.emplace(col, row, col + 1, row + 1);
						break;
					}
					default:
						LogError("Unexpected module id {} at {{{},{}}} in base {}",
						         (int)b.module[row][col], col, row, id);
				}
			}
		}
		if (!foundLift)
		{
			LogError("No lift found in base {}", id);
		}
		Rect<int>::compactRectSet(layout->baseCorridors);
		state.base_layouts[id] = layout;
	}
}

} // namespace OpenApoc
