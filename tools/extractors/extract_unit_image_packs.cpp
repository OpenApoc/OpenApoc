#include "game/state/battle/battleunitimagepack.h"
#include "game/state/gamestate.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/palette.h"
#include "tools/extractors/extractors.h"

namespace OpenApoc
{

sp<BattleUnitImagePack> InitialGameStateExtractor::extractImagePack(GameState &state, const UString &path, bool shadow)
{
	std::ignore = state;
	UString dirName = "xcom3/tacdata/";

	auto imageTabFileName = UString::format("%s%s.tab", dirName, path);
	auto imageTabFile = fw().data->fs.open(imageTabFileName);
	if (!imageTabFile)
	{
		LogError("Failed to open TAB file \"%s\"", imageTabFileName.cStr());
		return nullptr;
	}
	size_t imageTabFileEntryCount = imageTabFile.size() / 4;

	auto p = mksp<BattleUnitImagePack>();

	p->image_offset = shadow ? Vec2<float>{23, -14} : Vec2<float>{ 23, 34 };

	for (size_t i = 0; i < imageTabFileEntryCount; i++)
	{
		p->images.push_back(
			fw().data->loadImage(UString::format("%s:%s%s.pck:%s%s.tab:%u", shadow ? "PCKSHADOW" : "PCK", dirName, path, dirName, path, (unsigned)i))
		);
	}
	
	return p;
}

sp<BattleUnitImagePack> InitialGameStateExtractor::extractItemImagePack(GameState &state, int item)
{
	std::ignore = state;
	UString dirName = "xcom3/tacdata/";

	auto p = mksp<BattleUnitImagePack>();

	p->image_offset = { 23, 34 };

	for (int j = 0; j < 8; j++)
		p->images.push_back(fw().data->loadImage(
			UString::format("PCK:xcom3/tacdata/unit/equip.pck:xcom3/tacdata/"
				"unit/equip.tab:%d",
				item * 8 + j)));

	return p;
}

int InitialGameStateExtractor::getItemImagePacksCount()
{
	auto heldSpriteTabFileName = UString("xcom3/tacdata/unit/equip.tab");
	auto heldSpriteTabFile = fw().data->fs.open(heldSpriteTabFileName);
	if (!heldSpriteTabFile)
	{
		LogError("Failed to open held item sprite TAB file \"%s\"", heldSpriteTabFileName.cStr());
		return -1;
	}
	return heldSpriteTabFile.size() / 4 / 8;
}
}