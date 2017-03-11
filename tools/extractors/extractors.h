#pragma once

#include "game/state/battle/battlemappart_type.h"
#include "game/state/battle/battleunitanimationpack.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include "tools/extractors/common/animation.h"
#include "tools/extractors/common/tacp.h"
#include "tools/extractors/common/ufo2p.h"
#include <map>
#include <vector>

namespace OpenApoc
{

class BattleMapTileset;
class BattleUnitAnimationPack;
class GameState;
class City;
class BattleMapSectorTiles;

class InitialGameStateExtractor
{
	// Proper value MUST be 24 on x, because width is 48, but no matter how I look at it, I just
	// can't accept it.
	// Voxel Maps ARE better suited for an offest of 23, period!
	const Vec2<float> BATTLE_IMAGE_OFFSET = {23, 34};
	const Vec2<float> BATTLE_SHADOW_OFFSET = {23, 6};

	const Vec2<float> CITY_IMAGE_OFFSET = {32, 24};

  private:
	UFO2P ufo2p;
	TACP tacp;

  public:
	enum class Difficulty
	{
		DIFFICULTY_1,
		DIFFICULTY_2,
		DIFFICULTY_3,
		DIFFICULTY_4,
		DIFFICULTY_5,
	};
	InitialGameStateExtractor() = default;
	void extractCommon(GameState &state) const;
	void extract(GameState &state, Difficulty difficulty) const;
	/* extractBulletSprites() returns a list of images, so doesn't affect a GameState */
	std::map<UString, sp<Image>> extractBulletSpritesCity() const;
	std::map<UString, sp<Image>> extractBulletSpritesBattle() const;

	// Though this takes a gamestate, that's just used to hang StateRef<>s off
	sp<BattleMapTileset> extractTileSet(GameState &state, const UString &name) const;
	sp<BattleUnitImagePack> extractImagePack(GameState &state, const UString &path,
	                                         bool shadow) const;
	sp<BattleUnitImagePack> extractItemImagePack(GameState &state, int item) const;
	int getItemImagePacksCount() const;
	sp<BattleUnitAnimationPack> extractAnimationPack(GameState &state, const UString &path,
	                                                 const UString &name) const;
	std::map<UString, up<BattleMapSectorTiles>> extractMapSectors(GameState &state,
	                                                              const UString &mapRootName) const;

	// Lookup table of building function number -> battlemap path
	static const std::vector<UString> battleMapPaths;
	// Lookup table for battlemap path -> number of features that are converted to fires on map start
	static const std::map<OpenApoc::UString, std::set<int>> initialFires;
	// Lookup table for battlemap path -> number of features that are converted to smokes on map start
	static const std::map<OpenApoc::UString, std::set<int>> initialSmokes;
	// List of paths and names for unit image packs
	static const std::map<OpenApoc::UString, OpenApoc::UString> unitImagePackPaths;
	// List of paths and names for unit shadow packs
	static const std::map<OpenApoc::UString, OpenApoc::UString> unitShadowPackPaths;
	// List of paths and names for animation packs
	static const std::map<OpenApoc::UString, OpenApoc::UString> unitAnimationPackPaths;

  private:
	// 'common' state doesn't rely on difficulty
	void extractAgentTypes(GameState &state) const;
	void extractAgentBodyTypes(GameState &state) const;
	void extractVehicles(GameState &state) const;
	void extractOrganisations(GameState &state) const;
	void extractFacilities(GameState &state) const;
	void extractBaseLayouts(GameState &state) const;
	void extractVehicleEquipment(GameState &state) const;
	void extractResearch(GameState &state) const;
	void extractAgentEquipment(GameState &state) const;
	void extractDoodads(GameState &state) const;

	void extractBattlescapeMap(GameState &state, const std::vector<OpenApoc::UString> &paths) const;
	void extractBattlescapeMapFromPath(GameState &state, const UString dirName,
	                                   const int index) const;
	void readBattleMapParts(GameState &state, const TACP &data_t, sp<BattleMapTileset> t,
	                        BattleMapPartType::Type type, const UString &idPrefix, const UString &mapName,
	                        const UString &dirName, const UString &datName, const UString &pckName,
	                        const UString &stratPckName) const;

	void extractSharedBattleResources(GameState &state) const;

	// Then things that change on difficulty

	void extractAlienEquipmentSets(GameState &state, Difficulty difficulty) const;

	void extractBuildings(GameState &state, UString bldFileName, sp<City> city,
	                      bool alienBuilding = false) const;
	void extractCityMap(GameState &state, UString fileName, UString tilePrefix,
	                    sp<City> city) const;
	void extractCityScenery(GameState &state, UString tilePrefix, UString datFile,
	                        UString spriteFile, UString stratmapFile, UString lofFile,
	                        UString ovrFile, sp<City> city) const;

	// Unit animation packs functions

	sp<BattleUnitAnimationPack::AnimationEntry> getAnimationEntry(
	    const std::vector<AnimationDataAD> &dataAD, const std::vector<AnimationDataUA> &dataUA,
	    std::vector<AnimationDataUF> &dataUF, int index, Vec2<int> direction,
	    int frames_per_100_units, int split_point, bool left_side, bool isOverlay = false,
	    bool removeItem = false, Vec2<int> targetOffset = {0, 0}, Vec2<int> beginOffset = {0, 0},
	    bool inverse = false, int extraEndFrames = 0, bool singleFrame = false) const;

	sp<BattleUnitAnimationPack::AnimationEntry>
	getAnimationEntry(const std::vector<AnimationDataAD> &dataAD,
	                  const std::vector<AnimationDataUA> &dataUA,
	                  std::vector<AnimationDataUF> &dataUF, int index, Vec2<int> direction,
	                  int frames_per_100_units = 100, bool isOverlay = false,
	                  Vec2<int> targetOffset = {0, 0}, Vec2<int> beginOffset = {0, 0}) const
	{
		return getAnimationEntry(dataAD, dataUA, dataUF, index, direction, frames_per_100_units, 0,
		                         false, isOverlay, false, targetOffset, beginOffset);
	}

	sp<BattleUnitAnimationPack::AnimationEntry> getAnimationEntry(
	    const std::vector<AnimationDataAD> &dataAD, const std::vector<AnimationDataUA> &dataUA,
	    std::vector<AnimationDataUF> &dataUF, int index, Vec2<int> direction, bool inverse) const
	{
		return getAnimationEntry(dataAD, dataUA, dataUF, index, direction, 100, 0, false, false,
		                         false, {0, 0}, {0, 0}, inverse);
	}

	sp<BattleUnitAnimationPack::AnimationEntry>
	getAnimationEntry(const std::vector<AnimationDataAD> &dataAD,
	                  const std::vector<AnimationDataUA> &dataUA,
	                  std::vector<AnimationDataUF> &dataUF, int index, Vec2<int> direction,
	                  Vec2<int> targetOffset, Vec2<int> beginOffset) const
	{
		return getAnimationEntry(dataAD, dataUA, dataUF, index, direction, 100, 0, false, false,
		                         false, targetOffset, beginOffset);
	}

	Vec2<int> gPrOff(Vec2<int> facing) const;

	// Unit animation pack extractors

	// Template for copying to other files
	void extractAnimationPackTemplate(sp<BattleUnitAnimationPack> p,
								  const std::vector<AnimationDataAD> &dataAD,
								  const std::vector<AnimationDataUA> &dataUA,
								  std::vector<AnimationDataUF> &dataUF) const;

	void extractAnimationPackUnit(sp<BattleUnitAnimationPack> p,
								  const std::vector<AnimationDataAD> &dataAD,
								  const std::vector<AnimationDataUA> &dataUA,
								  std::vector<AnimationDataUF> &dataUF) const;

	void extractAnimationPackBsk(sp<BattleUnitAnimationPack> p,
		const std::vector<AnimationDataAD> &dataAD,
		const std::vector<AnimationDataUA> &dataUA,
		std::vector<AnimationDataUF> &dataUF) const;

};
}
