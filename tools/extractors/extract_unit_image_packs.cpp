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

	for (size_t i = 0; i < imageTabFileEntryCount; i++)
	{
		p->images.push_back(
			fw().data->loadImage(UString::format("%s:%s%s.pck:%s%s.tab:%u:%stactical.pal", shadow ? "PCKSHADOW" : "PCK", dirName, path, dirName, path, dirName, (unsigned)i))
		);
	}
	
	return p;
}

}