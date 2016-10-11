#include "framework/data.h"
#include "framework/framework.h"
#include "game/state/agent.h"
#include "game/state/battle/battlecommonimagelist.h"
#include "game/state/battle/battlecommonsamplelist.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/rules/damage.h"
#include "library/strings_format.h"
#include "tools/extractors/common/tacp.h"
#include "tools/extractors/extractors.h"
#include <limits>

namespace OpenApoc
{

void InitialGameStateExtractor::extractSharedBattleResources(GameState &state)
{
	// Common Images

	auto gameObjectStrategySpriteTabFileName = UString("xcom3/tacdata/stratico.tab");
	auto gameObjectStrategySpriteTabFile = fw().data->fs.open(gameObjectStrategySpriteTabFileName);
	if (!gameObjectStrategySpriteTabFile)
	{
		LogError("Failed to open dropped item StrategySprite TAB file \"%s\"",
		         gameObjectStrategySpriteTabFileName.cStr());
		return;
	}
	size_t gameObjectStrategySpriteCount = gameObjectStrategySpriteTabFile.size() / 4;

	state.battle_common_image_list = mksp<BattleCommonImageList>();

	state.battle_common_image_list->strategyImages = mksp<std::vector<sp<Image>>>();
	for (size_t i = 0; i < gameObjectStrategySpriteCount; i++)
	{
		state.battle_common_image_list->strategyImages->push_back(
		    fw().data->loadImage(format("PCKSTRAT:xcom3/tacdata/stratico.pck:xcom3/tacdata/"
		                                "stratico.tab:%u",
		                                (unsigned)i)));
	}

	state.battle_common_image_list->loadingImage =
	    fw().data->loadImage("xcom3/ufodata/enttact.pcx");

	state.battle_common_image_list->focusArrows.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                64)));
	state.battle_common_image_list->focusArrows.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                65)));
	state.battle_common_image_list->focusArrows.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                66)));
	state.battle_common_image_list->focusArrows.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                67)));

	// Common Sounds

	static const int SOUND_METAL = 0x01;
	static const int SOUND_SOFT = 0x02;
	static const int SOUND_MUD = 0x03;
	static const int SOUND_SLUDG = 0x04;
	static const int SOUND_WOOD = 0x05;
	static const int SOUND_MARB = 0x06;
	static const int SOUND_CONC = 0x07;
	static const int SOUND_TUBE = 0x08;

	state.battle_common_sample_list = mksp<BattleCommonSampleList>();

	state.battle_common_sample_list->gravlift =
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/gravlift.raw:22050");
	state.battle_common_sample_list->door =
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/terrain/doorwhsh.raw:22050");
	state.battle_common_sample_list->brainsuckerHatch =
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/extra/brhatch.raw:22050");
	state.battle_common_sample_list->teleport =
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/explosns/teleport.raw:22050");

	state.battle_common_sample_list->genericHitSounds = mksp<std::list<sp<Sample>>>();
	state.battle_common_sample_list->genericHitSounds->push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/hit1.raw:22050"));
	state.battle_common_sample_list->genericHitSounds->push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/hit2.raw:22050"));
	state.battle_common_sample_list->genericHitSounds->push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/hit3.raw:22050"));

	UString sfx_name = "";
	for (int i = 1; i <= 8; i++)
	{
		switch (i)
		{
			case SOUND_CONC:
				sfx_name = "conc";
				break;
			case SOUND_MARB:
				sfx_name = "marb";
				break;
			case SOUND_METAL:
				sfx_name = "metal";
				break;
			case SOUND_MUD:
				sfx_name = "mud";
				break;
			case SOUND_SLUDG:
				sfx_name = "sludg";
				break;
			case SOUND_SOFT:
				sfx_name = "soft";
				break;
			case SOUND_TUBE:
				sfx_name = "tube";
				break;
			case SOUND_WOOD:
				sfx_name = "wood";
				break;
		}

		state.battle_common_sample_list->walkSounds.push_back(mksp<std::vector<sp<Sample>>>());
		state.battle_common_sample_list->walkSounds[i - 1]->push_back(fw().data->loadSample(
		    format("RAWSOUND:xcom3/rawsound/extra/ft%s%d.raw:22050", sfx_name.cStr(), 1)));
		state.battle_common_sample_list->walkSounds[i - 1]->push_back(fw().data->loadSample(
		    format("RAWSOUND:xcom3/rawsound/extra/ft%s%d.raw:22050", sfx_name.cStr(), 2)));

		state.battle_common_sample_list->objectDropSounds.push_back(fw().data->loadSample(
		    format("RAWSOUND:xcom3/rawsound/extra/ob%s.raw:22050", sfx_name.cStr())));
	}

	state.battle_common_sample_list->throwSounds.push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/weapons/throw1.raw:22050"));
	state.battle_common_sample_list->throwSounds.push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/weapons/throw2.raw:22050"));
}
}
