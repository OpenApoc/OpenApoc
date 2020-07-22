#include "framework/data.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battlecommonimagelist.h"
#include "game/state/rules/battle/battlecommonsamplelist.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/rules/city/citycommonimagelist.h"
#include "game/state/rules/city/citycommonsamplelist.h"
#include "game/state/shared/agent.h"
#include "library/strings_format.h"
#include "library/voxel.h"
#include "tools/extractors/common/tacp.h"
#include "tools/extractors/extractors.h"
#include <limits>

namespace OpenApoc
{

void InitialGameStateExtractor::extractSharedCityResources(GameState &state) const
{
	state.city_common_image_list = mksp<CityCommonImageList>();
	state.city_common_image_list->strategyImages = mksp<std::vector<sp<Image>>>();
	for (size_t i = 544; i <= 589; i++)
	{
		state.city_common_image_list->strategyImages->push_back(
		    fw().data->loadImage(format("PCKSTRAT:xcom3/ufodata/stratmap.pck:xcom3/ufodata/"
		                                "stratmap.tab:%u",
		                                (unsigned)i)));
	}
	state.city_common_image_list->agentIsometric = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/icon_m.pck:xcom3/ufodata/icon_m.tab:%d:xcom3/ufodata/pal_01.dat", 26));
	state.city_common_image_list->agentStrategic =
	    fw().data->loadImage(format("PCKSTRAT:xcom3/ufodata/stratmap.pck:xcom3/ufodata/"
	                                "stratmap.tab:%d",
	                                571));
	for (int i = 586; i <= 589; i++)
	{
		state.city_common_image_list->portalStrategic.push_back(
		    fw().data->loadImage(format("PCKSTRAT:xcom3/ufodata/stratmap.pck:xcom3/ufodata/"
		                                "stratmap.tab:%d",
		                                i)));
	}
	state.city_common_image_list->projectileVoxelMap =
	    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
	for (int i = 6; i < 10; i++)
	{
		state.city_common_image_list->projectileVoxelMap->setSlice(
		    i, fw().data->loadVoxelSlice(format("LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
		                                        "ufodata/loftemps.tab:%d",
		                                        112)));
	}
	state.city_common_image_list->portalVoxelMap =
	    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
	for (int i = 0; i < 16; i++)
	{
		int index = 109;
		if (i < 4 || i >= 12)
		{
			index = 110;
		}
		state.city_common_image_list->portalVoxelMap->setSlice(
		    i, fw().data->loadVoxelSlice(format("LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
		                                        "ufodata/loftemps.tab:%d",
		                                        index)));
	}
}

void InitialGameStateExtractor::extractSharedBattleResources(GameState &state) const
{
	// Common Images

	auto gameObjectStrategySpriteTabFileName = UString("xcom3/tacdata/stratico.tab");
	auto gameObjectStrategySpriteTabFile = fw().data->fs.open(gameObjectStrategySpriteTabFileName);
	if (!gameObjectStrategySpriteTabFile)
	{
		LogError("Failed to open dropped item StrategySprite TAB file \"%s\"",
		         gameObjectStrategySpriteTabFileName);
		return;
	}
	size_t gameObjectStrategySpriteCount = gameObjectStrategySpriteTabFile.size() / 4;

	state.battle_common_image_list = mksp<BattleCommonImageList>();

	state.battle_common_image_list->burningDoodad = {&state, "DOODAD_16_BURNING_OBJECT"};

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

	state.city_common_sample_list = mksp<CityCommonSampleList>();

	state.city_common_sample_list->teleport =
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/strategc/explosns/teleport.raw:22050");
	state.city_common_sample_list->vehicleExplosion =
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/strategc/explosns/explosn2.raw:22050");
	state.city_common_sample_list->sceneryExplosion =
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/extra/expterr.raw:11025");
	state.city_common_sample_list->shieldHit =
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/strategc/explosns/shldhit1.raw:22050");
	state.city_common_sample_list->dimensionShiftIn =
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/strategc/terrain/dgate_in.raw:22050");
	state.city_common_sample_list->dimensionShiftOut =
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/strategc/terrain/dgat_out.raw:22050");
	state.city_common_sample_list->alertSounds.emplace_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/alert.raw:22050"));
	state.city_common_sample_list->alertSounds.emplace_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/alert2.raw:22050"));
	state.city_common_sample_list->alertSounds.emplace_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/alert3.raw:22050"));
	state.city_common_sample_list->alertSounds.emplace_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/alert4.raw:22050"));

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
	state.battle_common_sample_list->brainsuckerSuck =
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/aliens/attacks/brainsuk.raw:22050");
	state.battle_common_sample_list->teleport =
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/explosns/teleport.raw:22050");
	state.battle_common_sample_list->burn =
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/burning.raw:11025");

	state.battle_common_sample_list->genericHitSounds = mksp<std::list<sp<Sample>>>();
	state.battle_common_sample_list->genericHitSounds->push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/hit1.raw:22050"));
	state.battle_common_sample_list->genericHitSounds->push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/hit2.raw:22050"));
	state.battle_common_sample_list->genericHitSounds->push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/hit3.raw:22050"));

	state.battle_common_sample_list->psiSuccessSounds = mksp<std::list<sp<Sample>>>();
	state.battle_common_sample_list->psiSuccessSounds->push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/psionic1.raw:22050"));
	state.battle_common_sample_list->psiSuccessSounds->push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/psionic2.raw:22050"));

	state.battle_common_sample_list->psiFailSounds = mksp<std::list<sp<Sample>>>();
	state.battle_common_sample_list->psiFailSounds->push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/psionic4.raw:22050"));
	state.battle_common_sample_list->psiFailSounds->push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/psionic5.raw:22050"));
	state.battle_common_sample_list->psiFailSounds->push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/psionic6.raw:22050"));
	state.battle_common_sample_list->psiFailSounds->push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/zextra/psionic7.raw:22050"));

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
		    format("RAWSOUND:xcom3/rawsound/extra/ft%s%d.raw:22050", sfx_name, 1)));
		state.battle_common_sample_list->walkSounds[i - 1]->push_back(fw().data->loadSample(
		    format("RAWSOUND:xcom3/rawsound/extra/ft%s%d.raw:22050", sfx_name, 2)));

		state.battle_common_sample_list->objectDropSounds.push_back(fw().data->loadSample(
		    format("RAWSOUND:xcom3/rawsound/extra/ob%s.raw:22050", sfx_name)));
	}

	state.battle_common_sample_list->throwSounds.push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/weapons/throw1.raw:22050"));
	state.battle_common_sample_list->throwSounds.push_back(
	    fw().data->loadSample("RAWSOUND:xcom3/rawsound/tactical/weapons/throw2.raw:22050"));
}
} // namespace OpenApoc
