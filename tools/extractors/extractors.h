#pragma once

#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/rules/battle/battleunitanimationpack.h"
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

namespace
{
// Proper value MUST be 24 on x, because width is 48, but no matter how I look at it, I just
// can't accept it.
// Voxel Maps ARE better suited for an offset of 23, period!
const Vec2<float> BATTLE_IMAGE_OFFSET = {23, 34};
const Vec2<float> BATTLE_SHADOW_OFFSET = {23, 6};

const Vec2<float> CITY_IMAGE_OFFSET = {32, 24};
} // namespace

class BattleMapTileset;
class BattleUnitAnimationPack;
class GameState;
class City;
class BattleMapSectorTiles;

class InitialGameStateExtractor
{
  private:
	UFO2P ufo2p;
	TACP tacp;

  public:
	enum class Difficulty
	{
		DIFFICULTY_1 = 0,
		DIFFICULTY_2 = 1,
		DIFFICULTY_3 = 2,
		DIFFICULTY_4 = 3,
		DIFFICULTY_5 = 4,
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
	// Lookup table for battlemap path -> id of features that are converted to fires on start
	static const std::map<OpenApoc::UString, std::set<int>> initialFires;
	// Lookup table for battlemap path -> id of features that are converted to smokes on start
	static const std::map<OpenApoc::UString, std::set<int>> initialSmokes;
	// Lookup table for battlemap path -> id of ground that are spawning reinforcements
	static const std::map<OpenApoc::UString, std::set<int>> reinforcementSpawners;
	// Lookup table for battlemap path -> reinforcement timers
	static const std::map<OpenApoc::UString, int> reinforcementTimers;
	// Lookup table for battlemap path -> id of feature that is a mission objective (to destroy)
	static const std::map<OpenApoc::UString, std::set<int>> missionObjectives;
	// List of paths and names for unit image packs
	static const std::map<OpenApoc::UString, OpenApoc::UString> unitImagePackPaths;
	// List of paths and names for unit shadow packs
	static const std::map<OpenApoc::UString, OpenApoc::UString> unitShadowPackPaths;
	// List of paths and names for animation packs
	static const std::map<OpenApoc::UString, OpenApoc::UString> unitAnimationPackPaths;
	// Detection weights for building functions
	static const std::vector<int> buildingFunctionDetectionWeights;
	// Lookup value for tube NESW orientation
	static const std::map<OpenApoc::UString, std::vector<int>> tubes;

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
	void extractEconomy(GameState &state) const;

	void extractBattlescapeMap(GameState &state, const std::vector<OpenApoc::UString> &paths) const;
	void extractBattlescapeMapFromPath(GameState &state, const UString dirName,
	                                   const int index) const;
	void readBattleMapParts(GameState &state, const TACP &data_t, sp<BattleMapTileset> t,
	                        BattleMapPartType::Type type, const UString &idPrefix,
	                        const UString &mapName, const UString &dirName, const UString &datName,
	                        const UString &pckName, const UString &stratPckName) const;

	void extractSharedCityResources(GameState &state) const;

	void extractSharedBattleResources(GameState &state) const;

	void extractBuildingFunctions(GameState &state) const;

	// Then things that change on difficulty

	void extractAlienEquipmentSets(GameState &state, Difficulty difficulty) const;

	void extractBuildings(GameState &state, UString bldFileName, sp<City> city,
	                      bool alienBuilding = false) const;
	void extractCityMap(GameState &state, UString fileName, UString tilePrefix,
	                    sp<City> city) const;
	void extractCityScenery(GameState &state, UString tilePrefix, UString datFile,
	                        UString spriteFile, UString stratmapFile, UString lofFile,
	                        UString ovrFile, sp<City> city) const;

  public:
	// Unit animation packs functions

	sp<BattleUnitAnimationPack::AnimationEntry>
	combineAnimationEntries(sp<BattleUnitAnimationPack::AnimationEntry> e1,
	                        sp<BattleUnitAnimationPack::AnimationEntry> e2) const;

	sp<BattleUnitAnimationPack::AnimationEntry> getAnimationEntry(
	    const std::vector<AnimationDataAD> &dataAD, const std::vector<AnimationDataUA> &dataUA,
	    std::vector<AnimationDataUF> &dataUF, int index, Vec2<int> direction,
	    //
	    int units_per_100_frames, int split_point, bool left_side, bool isOverlay = false,
	    bool removeItem = false, Vec2<int> targetOffset = {0, 0}, Vec2<int> beginOffset = {0, 0},
	    bool inverse = false, int extraEndFrames = 0, bool singleFrame = false,
	    bool doubleFrames = false) const;

	sp<BattleUnitAnimationPack::AnimationEntry>
	getAnimationEntry(const std::vector<AnimationDataAD> &dataAD,
	                  const std::vector<AnimationDataUA> &dataUA,
	                  std::vector<AnimationDataUF> &dataUF, int index, Vec2<int> direction,
	                  //
	                  int units_per_100_frames = 100, bool isOverlay = false,
	                  Vec2<int> targetOffset = {0, 0}, Vec2<int> beginOffset = {0, 0}) const
	{
		return getAnimationEntry(dataAD, dataUA, dataUF, index, direction, units_per_100_frames, 0,
		                         false, isOverlay, false, targetOffset, beginOffset);
	}

	sp<BattleUnitAnimationPack::AnimationEntry>
	getAnimationEntry(const std::vector<AnimationDataAD> &dataAD,
	                  const std::vector<AnimationDataUA> &dataUA,
	                  std::vector<AnimationDataUF> &dataUF, int index, Vec2<int> direction,
	                  //
	                  Vec2<int> targetOffset, Vec2<int> beginOffset) const
	{
		return getAnimationEntry(dataAD, dataUA, dataUF, index, direction, 100, 0, false, false,
		                         false, targetOffset, beginOffset);
	}

	sp<BattleUnitAnimationPack::AnimationEntry>
	getAnimationEntryInv(const std::vector<AnimationDataAD> &dataAD,
	                     const std::vector<AnimationDataUA> &dataUA,
	                     std::vector<AnimationDataUF> &dataUF, int index, Vec2<int> direction,
	                     //
	                     int units_per_100_frames = 100, bool isOverlay = false,
	                     Vec2<int> targetOffset = {0, 0}, Vec2<int> beginOffset = {0, 0}) const
	{
		return getAnimationEntry(dataAD, dataUA, dataUF, index, direction, units_per_100_frames, 0,
		                         false, isOverlay, false, targetOffset, beginOffset, true);
	}

	sp<BattleUnitAnimationPack::AnimationEntry>
	getAnimationEntryDbl(const std::vector<AnimationDataAD> &dataAD,
	                     const std::vector<AnimationDataUA> &dataUA,
	                     std::vector<AnimationDataUF> &dataUF, int index, Vec2<int> direction,
	                     //
	                     int units_per_100_frames = 100, bool isOverlay = false,
	                     Vec2<int> targetOffset = {0, 0}, Vec2<int> beginOffset = {0, 0}) const
	{
		return getAnimationEntry(dataAD, dataUA, dataUF, index, direction, units_per_100_frames, 0,
		                         false, isOverlay, false, targetOffset, beginOffset, false, 0,
		                         false, true);
	}

	Vec2<int> gPrOff(Vec2<int> facing) const;

	sp<BattleUnitAnimationPack::AnimationEntry>
	makeUpAnimationEntry(int from, int count, int fromS, int countS, int partCount,
	                     Vec2<int> offset, int units_per_100_frames = 100) const;

	// This one automatically adjusts by direction
	sp<BattleUnitAnimationPack::AnimationEntry>
	makeUpAnimationEntry(int from, int count, int fromS, int countS, int partCount,
	                     Vec2<int> direction, Vec2<int> offset,
	                     int units_per_100_frames = 100) const
	{
		static const std::map<Vec2<int>, int> offset_dir_map = {
		    {{0, -1}, 4}, {{1, -1}, 5}, {{1, 0}, 6},  {{1, 1}, 7},
		    {{0, 1}, 0},  {{-1, 1}, 1}, {{-1, 0}, 2}, {{-1, -1}, 3},
		};
		from += offset_dir_map.at(direction) * count;
		fromS += offset_dir_map.at(direction) * countS;
		return makeUpAnimationEntry(from, count, fromS, countS, partCount, offset,
		                            units_per_100_frames);
	}

	// Unit animation pack extractors

  private:
	void extractAnimationPackUnit(sp<BattleUnitAnimationPack> p,
	                              const std::vector<AnimationDataAD> &dataAD,
	                              const std::vector<AnimationDataUA> &dataUA,
	                              std::vector<AnimationDataUF> &dataUF) const;

	void extractAnimationPackBsk(sp<BattleUnitAnimationPack> p,
	                             const std::vector<AnimationDataAD> &dataAD,
	                             const std::vector<AnimationDataUA> &dataUA,
	                             std::vector<AnimationDataUF> &dataUF) const;

	void extractAnimationPackMega(sp<BattleUnitAnimationPack> p,
	                              const std::vector<AnimationDataAD> &dataAD,
	                              const std::vector<AnimationDataUA> &dataUA,
	                              std::vector<AnimationDataUF> &dataUF) const;

	void extractAnimationPackPsi(sp<BattleUnitAnimationPack> p,
	                             const std::vector<AnimationDataAD> &dataAD,
	                             const std::vector<AnimationDataUA> &dataUA,
	                             std::vector<AnimationDataUF> &dataUF) const;

	void extractAnimationPackMulti(sp<BattleUnitAnimationPack> p,
	                               const std::vector<AnimationDataAD> &dataAD,
	                               const std::vector<AnimationDataUA> &dataUA,
	                               std::vector<AnimationDataUF> &dataUF) const;

	void extractAnimationPackHyper(sp<BattleUnitAnimationPack> p,
	                               const std::vector<AnimationDataAD> &dataAD,
	                               const std::vector<AnimationDataUA> &dataUA,
	                               std::vector<AnimationDataUF> &dataUF) const;

	void extractAnimationPackSpitter(sp<BattleUnitAnimationPack> p,
	                                 const std::vector<AnimationDataAD> &dataAD,
	                                 const std::vector<AnimationDataUA> &dataUA,
	                                 std::vector<AnimationDataUF> &dataUF) const;

	void extractAnimationPackCiv(sp<BattleUnitAnimationPack> p,
	                             const std::vector<AnimationDataAD> &dataAD,
	                             const std::vector<AnimationDataUA> &dataUA,
	                             std::vector<AnimationDataUF> &dataUF) const;

	void extractAnimationPackPopper(sp<BattleUnitAnimationPack> p) const;

	void extractAnimationPackMicro(sp<BattleUnitAnimationPack> p) const;

	void extractAnimationPackQ(sp<BattleUnitAnimationPack> p) const;

	void extractAnimationPackGun(sp<BattleUnitAnimationPack> p) const;

	void extractAnimationPackChrysalis(sp<BattleUnitAnimationPack> p, bool first) const;

	void extractAnimationPackEgg(sp<BattleUnitAnimationPack> p, bool first) const;
};
} // namespace OpenApoc
