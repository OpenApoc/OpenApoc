#include "framework/options.h"
#include "framework/logger.h"

namespace
{
using namespace OpenApoc;
void dumpOption(const ConfigOptionInt &opt)
{
	LogInfo("OPTION \"%s.%s\" = %d", opt.getSection(), opt.getName(), opt.get());
}
void dumpOption(const ConfigOptionBool &opt)
{
	LogInfo("OPTION \"%s.%s\" = %s", opt.getSection(), opt.getName(), opt.get() ? "true" : "false");
}
void dumpOption(const ConfigOptionFloat &opt)
{
	LogInfo("OPTION \"%s.%s\" = %f", opt.getSection(), opt.getName(), opt.get());
}
void dumpOption(const ConfigOptionString &opt)
{
	LogInfo("OPTION \"%s.%s\" = \"%s\"", opt.getSection(), opt.getName(), opt.get());
}

} // anonymous namespace

namespace OpenApoc::Options
{
void dumpOptionsToLog()
{
	dumpOption(dataPathOption);
	dumpOption(cdPathOption);
	dumpOption(threadPoolSizeOption);
	dumpOption(renderersOption);
	dumpOption(audioBackendsOption);
	dumpOption(audioGlobalGainOption);
	dumpOption(audioSampleGainOption);
	dumpOption(audioMusicGainOption);
	dumpOption(screenWidthOption);
	dumpOption(screenHeightOption);
	dumpOption(screenFullscreenOption);
	dumpOption(screenScaleXOption);
	dumpOption(screenScaleYOption);
	dumpOption(languageOption);

	dumpOption(targetFPS);
	dumpOption(frameLimit);
	dumpOption(swapInterval);

	dumpOption(autoScrollOption);
	dumpOption(actionMusicOption);
	dumpOption(autoExecuteOption);
	dumpOption(toolTipDelay);

	dumpOption(optionPauseOnUfoSpotted);
	dumpOption(optionPauseOnVehicleLightDamage);
	dumpOption(optionPauseOnVehicleModerateDamage);
	dumpOption(optionPauseOnVehicleHeavyDamage);
	dumpOption(optionPauseOnVehicleDestroyed);
	dumpOption(optionPauseOnVehicleEscaping);
	dumpOption(optionPauseOnVehicleNoAmmo);
	dumpOption(optionPauseOnVehicleLowFuel);
	dumpOption(optionPauseOnAgentDiedCity);
	dumpOption(optionPauseOnAgentArrived);
	dumpOption(optionPauseOnCargoArrived);
	dumpOption(optionPauseOnTransferArrived);
	dumpOption(optionPauseOnRecoveryArrived);
	dumpOption(optionPauseOnVehicleRepaired);
	dumpOption(optionPauseOnVehicleRearmed);
	dumpOption(optionPauseOnNotEnoughAmmo);
	dumpOption(optionPauseOnVehicleRefuelled);
	dumpOption(optionPauseOnNotEnoughFuel);
	dumpOption(optionPauseOnUnauthorizedVehicle);
	dumpOption(optionPauseOnHostileSpotted);
	dumpOption(optionPauseOnHostileDied);
	dumpOption(optionPauseOnUnknownDied);
	dumpOption(optionPauseOnAgentDiedBattle);
	dumpOption(optionPauseOnAgentBrainsucked);
	dumpOption(optionPauseOnAgentCriticallyWounded);
	dumpOption(optionPauseOnAgentBadlyInjured);
	dumpOption(optionPauseOnAgentInjured);
	dumpOption(optionPauseOnAgentUnderFire);
	dumpOption(optionPauseOnAgentUnconscious);
	dumpOption(optionPauseOnAgentLeftCombat);
	dumpOption(optionPauseOnAgentFrozen);
	dumpOption(optionPauseOnAgentBerserk);
	dumpOption(optionPauseOnAgentPanicked);
	dumpOption(optionPauseOnAgentPanicOver);
	dumpOption(optionPauseOnAgentPsiAttacked);
	dumpOption(optionPauseOnAgentPsiControlled);
	dumpOption(optionPauseOnAgentPsiOver);

	dumpOption(optionUFODamageModel);
	dumpOption(optionInstantExplosionDamage);
	dumpOption(optionGravliftSounds);
	dumpOption(optionNoInstantThrows);
	dumpOption(optionFerryChecksRelationshipWhenBuying);
	dumpOption(optionAllowManualCityTeleporters);
	dumpOption(optionAllowManualCargoFerry);
	dumpOption(optionAllowSoldierTaxiUse);
	dumpOption(optionAllowUnloadingClips);
	dumpOption(optionPayloadExplosion);
	dumpOption(optionDisplayUnitPaths);
	dumpOption(optionAdditionalUnitIcons);
	dumpOption(optionAllowForceFiringParallel);
	dumpOption(optionRequireLOSToMaintainPsi);
	dumpOption(optionAllowAttackingOwnedVehicles);
	dumpOption(optionCallExistingFerry);
	dumpOption(optionAlternateVehicleShieldSound);
	dumpOption(optionEnableAgentTemplates);
	dumpOption(optionStoreDroppedEquipment);
	dumpOption(optionFallingGroundVehicles);

	dumpOption(optionEnforceCargoLimits);
	dumpOption(optionAllowNearbyVehicleLootPickup);
	dumpOption(optionAllowBuildingLootDeposit);
	dumpOption(optionArmoredRoads);
	dumpOption(optionVanillaCityControls);
	dumpOption(optionCollapseRaidedBuilding);
	dumpOption(optionScrambleOnUnintentionalHit);
	dumpOption(optionMarketRight);
	dumpOption(optionDGCrashingVehicles);
	dumpOption(optionFuelCrashingVehicles);
	dumpOption(optionSkipTurbo);
	dumpOption(optionRunAndKneel);
	dumpOption(optionSeedRng);
	dumpOption(optionAutoReload);

	dumpOption(optionStunHostileAction);
	dumpOption(optionRaidHostileAction);
	dumpOption(optionBSKLauncherSound);
	dumpOption(optionInvulnerableRoads);
	dumpOption(optionATVTank);

	dumpOption(optionATVAPC);

	dumpOption(optionCrashingVehicles);

	dumpOption(optionScriptsList);

	dumpOption(optionInfiniteAmmoCheat);
	dumpOption(optionDamageInflictedMultiplierCheat);
	dumpOption(optionDamageReceivedMultiplierCheat);
	dumpOption(optionHostilesMultiplierCheat);
	dumpOption(optionStatGrowthMultiplierCheat);

	dumpOption(optionEnableTouchEvents);

	dumpOption(imageCacheSize);
	dumpOption(imageSetCacheSize);
	dumpOption(voxelCacheSize);
	dumpOption(fontStringCacheSize);
	dumpOption(paletteCacheSize);

	dumpOption(fileLogLevelOption);

	dumpOption(backtraceLogLevelOption);
	dumpOption(dialogLogLevelOption);

	dumpOption(defaultTooltipFont);

	dumpOption(useCRCChecksum);
	dumpOption(useSHA1Checksum);

	dumpOption(enableTrace);
	dumpOption(traceFile);

	dumpOption(saveDirOption);
	dumpOption(packSaveOption);

	dumpOption(skipIntroOption);
	dumpOption(loadGameOption);

	dumpOption(modList);
	dumpOption(modPath);

	dumpOption(asyncLoading);
}

#ifndef DATA_DIRECTORY
#define DATA_DIRECTORY "./data"
#endif

#ifndef RENDERERS
#ifdef _WIN32
#pragma message("WARNING: Using default renderer list")
#else
#warning RENDERERS not set - using default list
#endif
#define RENDERERS "GLES_3_0:GL_2_0"
#endif

ConfigOptionString dataPathOption("Framework", "Data", "The path containing OpenApoc data",
                                  "./data");
ConfigOptionString cdPathOption("Framework", "CD", "The path to the XCom:Apocalypse CD",
                                "./data/cd.iso");
ConfigOptionInt threadPoolSizeOption(
    "Framework", "ThreadPoolSize",
    "The number of threads to spawn for the threadpool (0 = queried num_cores)", 0);
ConfigOptionString renderersOption("Framework", "Renderers",
                                   "':' separated list of renderer backends (in preference order)",
                                   RENDERERS);
ConfigOptionString audioBackendsOption("Framework", "AudioBackends",
                                       "':' separated list of audio backends (in preference order)",
                                       "SDLRaw:null");
ConfigOptionInt audioGlobalGainOption("Framework.Audio", "GlobalGain", "Global audio gain (0-20)",
                                      20);
ConfigOptionInt audioSampleGainOption("Framework.Audio", "SampleGain", "Sample audio gain (0-20)",
                                      20);
ConfigOptionInt audioMusicGainOption("Framework.Audio", "MusicGain", "Music audio gain (0-20)", 20);
ConfigOptionInt screenWidthOption("Framework.Screen", "Width", "Initial screen width (in pixels)",
                                  1280);
ConfigOptionInt screenHeightOption("Framework.Screen", "Height",
                                   "Initial screen height (in pixels)", 720);
ConfigOptionBool screenFullscreenOption("Framework.Screen", "Fullscreen", "Enable fullscreen mode",
                                        false);
ConfigOptionInt screenScaleXOption("Framework.Screen", "ScaleX",
                                   "Scale screen in X direction by (percent)", 100);
ConfigOptionInt screenScaleYOption("Framework.Screen", "ScaleY",
                                   "Scale screen in Y direction by (percent)", 100);
ConfigOptionString languageOption("Framework", "Language",
                                  "The language used ingame (empty for system default)", "");

ConfigOptionInt targetFPS("Framework", "TargetFPS", "The target FPS count - affects game speed!",
                          60);
ConfigOptionInt frameLimit("Framework", "FrameLimit", "Quit after this many frames - 0 = unlimited",
                           0);
ConfigOptionInt swapInterval("Framework", "SwapInterval",
                             "Swap interval (0 = tear, 1 = wait for vsync", 0);

ConfigOptionBool autoScrollOption("Options.Misc", "AutoScroll", "Enable scrolling with mouse",
                                  true);
ConfigOptionBool actionMusicOption("Options.Misc", "ActionMusic",
                                   "Music changes according to action in battle", true);
ConfigOptionBool autoExecuteOption("Options.Misc", "AutoExecute",
                                   "Execute remaining orders when player presses end turn button",
                                   false);
ConfigOptionInt toolTipDelay("Options.Misc", "ToolTipDelay",
                             "Delay in milliseconds before showing tooltips (<= 0 to disable)",
                             500);

ConfigOptionBool optionPauseOnUfoSpotted("Notifications.City", "UfoSpotted", "UFO spotted", true);
ConfigOptionBool optionPauseOnVehicleLightDamage("Notifications.City", "VehicleLightDamage",
                                                 "Vehicle lightly damaged", true);
ConfigOptionBool optionPauseOnVehicleModerateDamage("Notifications.City", "VehicleModerateDamage",
                                                    "Vehicle moderately damaged", true);
ConfigOptionBool optionPauseOnVehicleHeavyDamage("Notifications.City", "VehicleHeavyDamage",
                                                 "Vehicle heavily damaged", true);
ConfigOptionBool optionPauseOnVehicleDestroyed("Notifications.City", "VehicleDestroyed",
                                               "Vehicle destroyed", true);
ConfigOptionBool optionPauseOnVehicleEscaping("Notifications.City", "VehicleEscaping",
                                              "Vehicle damaged and returning to base", true);
ConfigOptionBool optionPauseOnVehicleNoAmmo("Notifications.City", "VehicleNoAmmo",
                                            "Weapon out of ammo", true);
ConfigOptionBool optionPauseOnVehicleLowFuel("Notifications.City", "VehicleLowFuel",
                                             "Vehicle low on fuel", true);
ConfigOptionBool optionPauseOnAgentDiedCity("Notifications.City", "AgentDiedCity", "Agent has died",
                                            true);
ConfigOptionBool optionPauseOnAgentArrived("Notifications.City", "AgentArrived",
                                           "Agent arrived at base", true);
ConfigOptionBool optionPauseOnCargoArrived("Notifications.City", "CargoArrived",
                                           "Cargo has arrived at base", true);
ConfigOptionBool optionPauseOnTransferArrived("Notifications.City", "TransferArrived",
                                              "Transfer arrived at base", true);
ConfigOptionBool optionPauseOnRecoveryArrived("Notifications.City", "RecoveryArrived",
                                              "Crash recovery arrived at base", true);
ConfigOptionBool optionPauseOnVehicleRepaired("Notifications.City", "VehicleRepaired",
                                              "Vehicle repaired", true);
ConfigOptionBool optionPauseOnVehicleRearmed("Notifications.City", "VehicleRearmed",
                                             "Vehicle rearmed", true);
ConfigOptionBool optionPauseOnNotEnoughAmmo("Notifications.City", "NotEnoughAmmo",
                                            "Not enough ammo to rearm vehicle", true);
ConfigOptionBool optionPauseOnVehicleRefuelled("Notifications.City", "VehicleRefuelled",
                                               "Vehicle refuelled", true);
ConfigOptionBool optionPauseOnNotEnoughFuel("Notifications.City", "NotEnoughFuel",
                                            "Not enough fuel to refuel vehicle", true);
ConfigOptionBool optionPauseOnUnauthorizedVehicle("Notifications.City", "UnauthorizedVehicle",
                                                  "Unauthorized vehicle detected", true);
ConfigOptionBool optionPauseOnHostileSpotted("Notifications.Battle", "HostileSpotted",
                                             "Hostile unit spotted", true);
ConfigOptionBool optionPauseOnHostileDied("Notifications.Battle", "HostileDied",
                                          "Hostile unit has died", true);
ConfigOptionBool optionPauseOnUnknownDied("Notifications.Battle", "UnknownDied",
                                          "Unknown Unit has died", true);
ConfigOptionBool optionPauseOnAgentDiedBattle("Notifications.Battle", "AgentDiedBattle",
                                              "Unit has died", true);
ConfigOptionBool optionPauseOnAgentBrainsucked("Notifications.Battle", "AgentBrainsucked",
                                               "Unit Brainsucked", true);
ConfigOptionBool optionPauseOnAgentCriticallyWounded("Notifications.Battle",
                                                     "AgentCriticallyWounded",
                                                     "Unit critically wounded", true);
ConfigOptionBool optionPauseOnAgentBadlyInjured("Notifications.Battle", "AgentBadlyInjured",
                                                "Unit badly injured", true);
ConfigOptionBool optionPauseOnAgentInjured("Notifications.Battle", "AgentInjured", "Unit injured",
                                           true);
ConfigOptionBool optionPauseOnAgentUnderFire("Notifications.Battle", "AgentUnderFire",
                                             "Unit under fire", true);
ConfigOptionBool optionPauseOnAgentUnconscious("Notifications.Battle", "AgentUnconscious",
                                               "Unit has lost consciousness", true);
ConfigOptionBool optionPauseOnAgentLeftCombat("Notifications.Battle", "AgentLeftCombat",
                                              "Unit has left combat zone", true);
ConfigOptionBool optionPauseOnAgentFrozen("Notifications.Battle", "AgentFrozen", "Unit has frozen",
                                          true);
ConfigOptionBool optionPauseOnAgentBerserk("Notifications.Battle", "AgentBerserk",
                                           "Unit has gone beserk", true);
ConfigOptionBool optionPauseOnAgentPanicked("Notifications.Battle", "AgentPanicked",
                                            "Unit has panicked", true);
ConfigOptionBool optionPauseOnAgentPanicOver("Notifications.Battle", "AgentPanicOver",
                                             "Unit has stopped panicking", true);
ConfigOptionBool optionPauseOnAgentPsiAttacked("Notifications.Battle", "AgentPsiAttacked",
                                               "Psionic attack on unit", true);
ConfigOptionBool optionPauseOnAgentPsiControlled("Notifications.Battle", "AgentPsiControlled",
                                                 "Unit under Psionic control", true);
ConfigOptionBool optionPauseOnAgentPsiOver("Notifications.Battle", "AgentPsiOver",
                                           "Unit freed from Psionic control", true);

ConfigOptionBool optionUFODamageModel("OpenApoc.NewFeature", "UFODamageModel",
                                      "X-Com 1 Damage model (0-200%)", false);
ConfigOptionBool optionInstantExplosionDamage("OpenApoc.NewFeature", "InstantExplosionDamage",
                                              "Explosions damage instantly", false);
ConfigOptionBool optionGravliftSounds("OpenApoc.NewFeature", "GravliftSounds", "Gravlift sounds",
                                      true);
ConfigOptionBool optionNoInstantThrows("OpenApoc.NewFeature", "NoInstantThrows",
                                       "Throwing requires proper facing and pose", true);
ConfigOptionBool optionFerryChecksRelationshipWhenBuying(
    "OpenApoc.NewFeature", "FerryChecksRelationshipWhenBuying",
    "Transtellar checks relationship when buying items", true);
ConfigOptionBool optionAllowManualCityTeleporters("OpenApoc.NewFeature",
                                                  "AllowManualCityTeleporters",
                                                  "Allow manual use of teleporters in city", true);
ConfigOptionBool optionAllowManualCargoFerry("OpenApoc.NewFeature", "AllowManualCargoFerry",
                                             "Allow manual ferrying of cargo and non-combatants",
                                             true);
ConfigOptionBool optionAllowSoldierTaxiUse("OpenApoc.NewFeature", "AllowSoldierTaxiUse",
                                           "Allow soldiers to call taxi", true);
ConfigOptionBool optionAllowUnloadingClips("OpenApoc.NewFeature", "AdvancedInventoryControls",
                                           "Allow unloading clips and quick equip", true);
ConfigOptionBool optionPayloadExplosion("OpenApoc.NewFeature", "PayloadExplosion",
                                        "Ammunition explodes when blown up", true);
ConfigOptionBool optionDisplayUnitPaths("OpenApoc.NewFeature", "DisplayUnitPaths",
                                        "Display unit paths in battle", true);
ConfigOptionBool optionAdditionalUnitIcons("OpenApoc.NewFeature", "AdditionalUnitIcons",
                                           "Display additional unit icons (fatal, psi)", true);
ConfigOptionBool optionAllowForceFiringParallel("OpenApoc.NewFeature", "AllowForceFiringParallel",
                                                "Allow force-firing parallel to the ground", true);
ConfigOptionBool optionRequireLOSToMaintainPsi("OpenApoc.NewFeature", "RequireLOSToMaintainPsi",
                                               "Require LOS to maintain psi attack", true);
ConfigOptionBool optionAllowAttackingOwnedVehicles("OpenApoc.NewFeature",
                                                   "AllowAttackingOwnedVehicles",
                                                   "Allow attacking owned vehicles", true);
ConfigOptionBool optionCallExistingFerry("OpenApoc.NewFeature", "CallExistingFerry",
                                         "Call existing transport instead of spawning them", true);
ConfigOptionBool
    optionAlternateVehicleShieldSound("OpenApoc.NewFeature", "AlternateVehicleShieldSound",
                                      "Hitting vehicle shield produces alternate sound", true);
ConfigOptionBool optionEnableAgentTemplates("OpenApoc.NewFeature", "EnableAgentTemplates",
                                            "Enable agent equipment templates", true);
ConfigOptionBool optionStoreDroppedEquipment("OpenApoc.NewFeature", "StoreDroppedEquipment",
                                             "Attempt to recover agent equipment dropped in city",
                                             true);
ConfigOptionBool optionFallingGroundVehicles("OpenApoc.NewFeature", "CrashingGroundVehicles",
                                             "Unsupported ground vehicles crash", true);

ConfigOptionBool optionEnforceCargoLimits("OpenApoc.NewFeature", "EnforceCargoLimits",
                                          "Enforce vehicle cargo limits", false);
ConfigOptionBool optionAllowNearbyVehicleLootPickup("OpenApoc.NewFeature",
                                                    "AllowNearbyVehicleLootPickup",
                                                    "Allow nearby vehicles to pick up loot", true);
ConfigOptionBool optionAllowBuildingLootDeposit("OpenApoc.NewFeature", "AllowBuildingLootDeposit",
                                                "Allow loot to be stashed in the building", true);
ConfigOptionBool optionArmoredRoads("OpenApoc.NewFeature", "ArmoredRoads", "Armored roads", true);
ConfigOptionBool optionVanillaCityControls("OpenApoc.NewFeature", "OpenApocCityControls",
                                           "Improved city control scheme", true);
ConfigOptionBool optionCollapseRaidedBuilding("OpenApoc.NewFeature", "CollapseRaidedBuilding",
                                              "Successful raid collapses building", true);
ConfigOptionBool
    optionScrambleOnUnintentionalHit("OpenApoc.NewFeature", "ScrambleOnUnintentionalHit",
                                     "Any hit on hostile building provokes retaliation", false);
ConfigOptionBool optionMarketRight("OpenApoc.NewFeature", "MarketOnRight",
                                   "Put market stock on the right side", true);
ConfigOptionBool optionDGCrashingVehicles("OpenApoc.NewFeature", "CrashingDimensionGate",
                                          "Uncapable vehicles crash when entering gates", true);
ConfigOptionBool optionFuelCrashingVehicles("OpenApoc.NewFeature", "CrashingOutOfFuel",
                                            "Vehicles crash when out of fuel", true);
ConfigOptionBool optionSkipTurbo("OpenApoc.NewFeature", "SkipTurboMovement",
                                 "Skip turbo movement calculations", false);
ConfigOptionBool optionRunAndKneel("OpenApoc.NewFeature", "RunAndKneel",
                                   "All units run and kneel by default", false);
ConfigOptionBool optionSeedRng("OpenApoc.NewFeature", "SeedRng", "Seed RNG on game start", true);
ConfigOptionBool optionAutoReload("OpenApoc.NewFeature", "AutoReload",
                                  "Automatically reload weapons when empty", true);

ConfigOptionBool optionStunHostileAction("OpenApoc.Mod", "StunHostileAction",
                                         "Stunning hurts relationships", false);
ConfigOptionBool optionRaidHostileAction("OpenApoc.Mod", "RaidHostileAction",
                                         "Initiating raid hurts relationships", false);
ConfigOptionBool optionBSKLauncherSound("OpenApoc.Mod", "BSKLauncherSound",
                                        "(MOD) Original Brainsucker Launcher SFX", true);
ConfigOptionBool optionInvulnerableRoads("OpenApoc.Mod", "InvulnerableRoads",
                                         "(MOD) Invulnerable roads", false);
ConfigOptionBool optionATVTank("OpenApoc.Mod", "ATVTank", "(MOD) Griffon becomes All-Terrain",
                               true);

ConfigOptionBool optionATVAPC("OpenApoc.Mod", "ATVAPC", "(MOD) Wolfhound APC becomes All-Terrain",
                              true);

ConfigOptionBool optionCrashingVehicles("OpenApoc.Mod", "CrashingVehicles",
                                        "Vehicles crash on low HP", false);

ConfigOptionString optionScriptsList("OpenApoc.Mod", "ScriptsList",
                                     "Semicolon-separated list of scripts to load",
                                     "data/scripts/openapoc_base.lua;");

ConfigOptionBool optionInfiniteAmmoCheat("OpenApoc.Cheat", "InfiniteAmmo",
                                         "Infinite ammo for X-Com agents and vehicles", false);
ConfigOptionFloat optionDamageInflictedMultiplierCheat("OpenApoc.Cheat",
                                                       "DamageInflictedMultiplier",
                                                       "Multiplier for damage inflicted by X-com",
                                                       1.0);
ConfigOptionFloat optionDamageReceivedMultiplierCheat("OpenApoc.Cheat", "DamageReceivedMultiplier",
                                                      "Multiplier for damage received by X-com",
                                                      1.0);
ConfigOptionFloat optionHostilesMultiplierCheat("OpenApoc.Cheat", "HostilesMultiplier",
                                                "Multiplier for number of hostiles", 1.0f);
ConfigOptionFloat optionStatGrowthMultiplierCheat("OpenApoc.Cheat", "StatGrowthMultiplier",
                                                  "Multiplier for agent stat growth", 1.0f);

ConfigOptionBool optionEnableTouchEvents("Framework", "EnableTouchEvents", "Enable touch events",
#ifdef ANDROID
                                         true
#else
                                         false
#endif
);

ConfigOptionInt imageCacheSize("Framework.Data", "ImageCacheSize",
                               "Number of Images to keep in data cache", 100);
ConfigOptionInt imageSetCacheSize("Framework.Data", "ImageSetCacheSize",
                                  "Number of ImageSets to keep in data cache", 10);
ConfigOptionInt voxelCacheSize("Framework.Data", "VoxelCacheSize",
                               "Number of VoxelMaps to keep in data cache", 1);
ConfigOptionInt fontStringCacheSize("Framework.Data", "FontStringCacheSize",
                                    "Number of rendered font stings to keep in data cache", 100);
ConfigOptionInt paletteCacheSize("Framework.Data", "PaletteCacheSize",
                                 "Number of Palettes to keep in data cache", 10);

ConfigOptionInt fileLogLevelOption(
    "Logger", "FileLevel",
    "Loglevel to output to file (0 = nothing, 1 = error, 2 = warning, 3 = info, 4 = debug)", 3);

ConfigOptionInt backtraceLogLevelOption("Logger", "BacktraceLevel",
                                        "Loglevel to print a backtrace to file log (0 = nothing, 1 "
                                        "= error, 2 = warning, 3 = info, 4 = debug)",
                                        1);

ConfigOptionInt dialogLogLevelOption(
    "Logger", "dialogLevel",
    "Loglevel to pop up a dialog(0 = nothing, 1 = error, 2 = warning, 3 = info, 4 = debug) ", 1);

ConfigOptionString defaultTooltipFont("Forms", "TooltipFont", "The default tooltip font",
                                      "smallset");
ConfigOptionBool useCRCChecksum("Framework.Serialization", "CRC",
                                "use a CRC checksum when saving files", false);
ConfigOptionBool useSHA1Checksum("Framework.Serialization", "SHA1",
                                 "use a SHA1 checksum when saving files", false);

ConfigOptionBool enableTrace("Trace", "enable", "Enable json call/time tracking");
ConfigOptionString traceFile("Trace", "outputFile", "File to output trace json to",
                             "openapoc.trace");

ConfigOptionString saveDirOption("Game.Save", "Directory", "Directory containing saved games",
                                 "./saves");
ConfigOptionBool packSaveOption("Game.Save", "Pack", "Pack saved games into a zip", true);

ConfigOptionBool skipIntroOption("Game", "SkipIntro", "Skip intro video", false);
ConfigOptionString loadGameOption("Game", "Load", "Path to save game to load at startup", "");

ConfigOptionString modList("Game", "Mods",
                           "A colon-separated list of mods to load (relative to mod directory)",
                           "base");
ConfigOptionString modPath("Game", "ModPath", "Directory containing mods", "./data/mods");
ConfigOptionBool asyncLoading("Game", "ASyncLoading",
                              "Load in background while displaying animated loading screen", true);

} // namespace OpenApoc::Options
