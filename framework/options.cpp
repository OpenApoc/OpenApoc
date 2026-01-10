#include "framework/options.h"
#include "framework/logger.h"
#include "library/strings_format.h"

namespace
{
using namespace OpenApoc;
void dumpOption(const ConfigOptionInt &opt)
{
	LogInfo("OPTION \"{0}.{1}\" = {2}", opt.getSection(), opt.getName(), opt.get());
}
void dumpOption(const ConfigOptionBool &opt)
{
	LogInfo("OPTION \"{0}.{1}\" = {2}", opt.getSection(), opt.getName(),
	        opt.get() ? "true" : "false");
}
void dumpOption(const ConfigOptionFloat &opt)
{
	LogInfo("OPTION \"{0}.{1}\" = {2}", opt.getSection(), opt.getName(), opt.get());
}
void dumpOption(const ConfigOptionString &opt)
{
	LogInfo("OPTION \"{0}.{1}\" = \"{2}\"", opt.getSection(), opt.getName(), opt.get());
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
	dumpOption(audioConcurrentSampleCount);
	dumpOption(screenWidthOption);
	dumpOption(screenHeightOption);
	dumpOption(screenFullscreenOption);
	dumpOption(screenModeOption);
	dumpOption(screenDisplayNumberOption);
	dumpOption(screenScaleXOption);
	dumpOption(screenScaleYOption);
	dumpOption(screenAutoScale);
	dumpOption(languageOption);
	dumpOption(mouseCaptureOption);

	dumpOption(targetFPS);
	dumpOption(frameLimit);
	dumpOption(swapInterval);

	dumpOption(autoScrollOption);
	dumpOption(actionMusicOption);
	dumpOption(autoExecuteOption);
	dumpOption(toolTipDelay);
	dumpOption(vanillaToggle);

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
	dumpOption(optionPauseOnBaseDestroyed);
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

	dumpOption(optionDebugCommandsVisible);
	dumpOption(optionUFODamageModel);
	dumpOption(optionInstantExplosionDamage);
	dumpOption(optionGravliftSounds);
	dumpOption(optionNoScrollSounds);
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
	dumpOption(optionLeftClickIcon);
	dumpOption(optionBattlescapeVertScroll);
	dumpOption(optionSingleSquadSelect);
	dumpOption(optionATVUFOMission);
	dumpOption(optionMaxTileRepair);
	dumpOption(optionSceneryRepairCostFactor);
	dumpOption(optionLoadSameAmmo);
	dumpOption(optionShowCurrentDimensionVehicles);
	dumpOption(optionShowNonXCOMVehiclesPrefix);
	dumpOption(isoOnlyFollow);
	dumpOption(formatAsCurrency);

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

ConfigOptionString dataPathOption("Framework", "Data", tr("The path containing OpenApoc data"),
                                  "./data");
ConfigOptionString cdPathOption("Framework", "CD", tr("The path to the XCom:Apocalypse CD"),
                                "./data/cd.iso");
ConfigOptionInt threadPoolSizeOption(
    "Framework", "ThreadPoolSize",
    tr("The number of threads to spawn for the threadpool (0 = queried num_cores)"), 0);
ConfigOptionString
    renderersOption("Framework", "Renderers",
                    tr("':' separated list of renderer backends (in preference order)"), RENDERERS);
ConfigOptionString
    audioBackendsOption("Framework", "AudioBackends",
                        tr("':' separated list of audio backends (in preference order)"),
                        "SDLRaw:null");
ConfigOptionInt audioGlobalGainOption("Framework.Audio", "GlobalGain",
                                      tr("Global audio gain (0-20)"), 20);
ConfigOptionInt audioSampleGainOption("Framework.Audio", "SampleGain",
                                      tr("Sample audio gain (0-20)"), 20);
ConfigOptionInt audioMusicGainOption("Framework.Audio", "MusicGain", tr("Music audio gain (0-20)"),
                                     20);
ConfigOptionInt
    audioConcurrentSampleCount("Framework.Audio", "ConcurrentSamples",
                               tr("The number of concurrent samples to play at one time"), 10);
ConfigOptionInt screenWidthOption("Framework.Screen", "Width",
                                  tr("Initial screen width (in pixels)"), 1280);
ConfigOptionInt screenHeightOption("Framework.Screen", "Height",
                                   tr("Initial screen height (in pixels)"), 720);
ConfigOptionBool screenFullscreenOption("Framework.Screen", "Fullscreen",
                                        tr("Deprecated: use ScreenMode instead"), false);
ConfigOptionString screenModeOption("Framework.Screen", "Mode",
                                    tr("Mode: {windowed,fullscreen,borderless}"), "windowed");
ConfigOptionInt screenDisplayNumberOption("Framework.Screen", "Display",
                                          tr("Display number in multi-monitor setup (0..n)"), 0);
ConfigOptionInt screenScaleXOption("Framework.Screen", "ScaleX",
                                   tr("Scale screen in X direction by (percent)"), 100);
ConfigOptionInt screenScaleYOption("Framework.Screen", "ScaleY",
                                   tr("Scale screen in Y direction by (percent)"), 100);
ConfigOptionBool screenAutoScale(
    "Framework.Screen", "AutoScale",
    tr("Automatically scale up game viewport for modern screens (overrides ScaleX and ScaleY)"),
    false);
ConfigOptionString languageOption("Framework", "Language",
                                  tr("The language used ingame (empty for system default)"), "");

ConfigOptionBool mouseCaptureOption("Framework", "MouseCapture",
                                    tr("Enable mouse capture for the window"), false);

ConfigOptionInt targetFPS("Framework", "TargetFPS",
                          tr("The target FPS count - affects game speed!"), 60);
ConfigOptionInt frameLimit("Framework", "FrameLimit",
                           tr("Quit after this many frames - 0 = unlimited"), 0);
ConfigOptionInt swapInterval("Framework", "SwapInterval",
                             tr("Swap interval (0 = tear, 1 = wait for vsync"), 0);

ConfigOptionBool autoScrollOption("Options.Misc", "AutoScroll", tr("Enable scrolling with mouse"),
                                  true);
ConfigOptionBool actionMusicOption("Options.Misc", "ActionMusic",
                                   tr("Music changes according to action in battle"), true);
ConfigOptionBool
    autoExecuteOption("Options.Misc", "AutoExecute",
                      tr("Execute remaining orders when player presses end turn button"), false);
ConfigOptionInt toolTipDelay("Options.Misc", "ToolTipDelay",
                             tr("Delay in milliseconds before showing tooltips (<= 0 to disable)"),
                             500);
ConfigOptionBool vanillaToggle("Options.Misc", "VanillaToggle", tr("Toggle vanilla mode"), false);

ConfigOptionBool optionPauseOnUfoSpotted("Notifications.City", "UfoSpotted", tr("UFO spotted"),
                                         true);
ConfigOptionBool optionPauseOnVehicleLightDamage("Notifications.City", "VehicleLightDamage",
                                                 tr("Vehicle lightly damaged"), true);
ConfigOptionBool optionPauseOnVehicleModerateDamage("Notifications.City", "VehicleModerateDamage",
                                                    tr("Vehicle moderately damaged"), true);
ConfigOptionBool optionPauseOnVehicleHeavyDamage("Notifications.City", "VehicleHeavyDamage",
                                                 tr("Vehicle heavily damaged"), true);
ConfigOptionBool optionPauseOnVehicleDestroyed("Notifications.City", "VehicleDestroyed",
                                               tr("Vehicle destroyed"), true);
ConfigOptionBool optionPauseOnVehicleEscaping("Notifications.City", "VehicleEscaping",
                                              tr("Vehicle damaged and returning to base"), true);
ConfigOptionBool optionPauseOnVehicleNoAmmo("Notifications.City", "VehicleNoAmmo",
                                            tr("Weapon out of ammo"), true);
ConfigOptionBool optionPauseOnVehicleLowFuel("Notifications.City", "VehicleLowFuel",
                                             tr("Vehicle low on fuel"), true);
ConfigOptionBool optionPauseOnAgentDiedCity("Notifications.City", "AgentDiedCity",
                                            tr("Agent has died"), true);
ConfigOptionBool optionPauseOnAgentArrived("Notifications.City", "AgentArrived",
                                           tr("Agent arrived at base"), true);
ConfigOptionBool optionPauseOnCargoArrived("Notifications.City", "CargoArrived",
                                           tr("Cargo has arrived at base"), true);
ConfigOptionBool optionPauseOnTransferArrived("Notifications.City", "TransferArrived",
                                              tr("Transfer arrived at base"), true);
ConfigOptionBool optionPauseOnRecoveryArrived("Notifications.City", "RecoveryArrived",
                                              tr("Crash recovery arrived at base"), true);
ConfigOptionBool optionPauseOnVehicleRepaired("Notifications.City", "VehicleRepaired",
                                              tr("Vehicle repaired"), true);
ConfigOptionBool optionPauseOnVehicleRearmed("Notifications.City", "VehicleRearmed",
                                             tr("Vehicle rearmed"), true);
ConfigOptionBool optionPauseOnNotEnoughAmmo("Notifications.City", "NotEnoughAmmo",
                                            tr("Not enough ammo to rearm vehicle"), true);
ConfigOptionBool optionPauseOnVehicleRefuelled("Notifications.City", "VehicleRefuelled",
                                               tr("Vehicle refuelled"), true);
ConfigOptionBool optionPauseOnNotEnoughFuel("Notifications.City", "NotEnoughFuel",
                                            tr("Not enough fuel to refuel vehicle"), true);
ConfigOptionBool optionPauseOnUnauthorizedVehicle("Notifications.City", "UnauthorizedVehicle",
                                                  tr("Unauthorized vehicle detected"), true);
ConfigOptionBool optionPauseOnBaseDestroyed("Notifications.City", "BaseDestroyed",
                                            tr("X-COM base destroyed by hostile forces."), true);
ConfigOptionBool optionPauseOnHostileSpotted("Notifications.Battle", "HostileSpotted",
                                             tr("Hostile unit spotted"), true);
ConfigOptionBool optionPauseOnHostileDied("Notifications.Battle", "HostileDied",
                                          tr("Hostile unit has died"), true);
ConfigOptionBool optionPauseOnUnknownDied("Notifications.Battle", "UnknownDied",
                                          tr("Unknown Unit has died"), true);
ConfigOptionBool optionPauseOnAgentDiedBattle("Notifications.Battle", "AgentDiedBattle",
                                              tr("Unit has died"), true);
ConfigOptionBool optionPauseOnAgentBrainsucked("Notifications.Battle", "AgentBrainsucked",
                                               tr("Unit Brainsucked"), true);
ConfigOptionBool optionPauseOnAgentCriticallyWounded("Notifications.Battle",
                                                     "AgentCriticallyWounded",
                                                     tr("Unit critically wounded"), true);
ConfigOptionBool optionPauseOnAgentBadlyInjured("Notifications.Battle", "AgentBadlyInjured",
                                                tr("Unit badly injured"), true);
ConfigOptionBool optionPauseOnAgentInjured("Notifications.Battle", "AgentInjured",
                                           tr("Unit injured"), true);
ConfigOptionBool optionPauseOnAgentUnderFire("Notifications.Battle", "AgentUnderFire",
                                             tr("Unit under fire"), true);
ConfigOptionBool optionPauseOnAgentUnconscious("Notifications.Battle", "AgentUnconscious",
                                               tr("Unit has lost consciousness"), true);
ConfigOptionBool optionPauseOnAgentLeftCombat("Notifications.Battle", "AgentLeftCombat",
                                              tr("Unit has left combat zone"), true);
ConfigOptionBool optionPauseOnAgentFrozen("Notifications.Battle", "AgentFrozen",
                                          tr("Unit has frozen"), true);
ConfigOptionBool optionPauseOnAgentBerserk("Notifications.Battle", "AgentBerserk",
                                           tr("Unit has gone beserk"), true);
ConfigOptionBool optionPauseOnAgentPanicked("Notifications.Battle", "AgentPanicked",
                                            tr("Unit has panicked"), true);
ConfigOptionBool optionPauseOnAgentPanicOver("Notifications.Battle", "AgentPanicOver",
                                             tr("Unit has stopped panicking"), true);
ConfigOptionBool optionPauseOnAgentPsiAttacked("Notifications.Battle", "AgentPsiAttacked",
                                               tr("Psionic attack on unit"), true);
ConfigOptionBool optionPauseOnAgentPsiControlled("Notifications.Battle", "AgentPsiControlled",
                                                 tr("Unit under Psionic control"), true);
ConfigOptionBool optionPauseOnAgentPsiOver("Notifications.Battle", "AgentPsiOver",
                                           tr("Unit freed from Psionic control"), true);
ConfigOptionBool optionDebugCommandsVisible("OpenApoc.NewFeature", "DebugCommandsVisible",
                                            tr("Show the debug commands on screen"), true);
ConfigOptionBool optionUFODamageModel("OpenApoc.NewFeature", "UFODamageModel",
                                      tr("X-Com 1 Damage model (0-200%)"), false);
ConfigOptionBool optionInstantExplosionDamage("OpenApoc.NewFeature", "InstantExplosionDamage",
                                              tr("Explosions damage instantly"), true);
ConfigOptionBool optionGravliftSounds("OpenApoc.NewFeature", "GravliftSounds",
                                      tr("Gravlift sounds"), true);
ConfigOptionBool optionNoScrollSounds("OpenApoc.NewFeature", "NoScrollSounds",
                                      tr("Disable scrolling sounds"), false);
ConfigOptionBool optionNoInstantThrows("OpenApoc.NewFeature", "NoInstantThrows",
                                       tr("Throwing requires proper facing and pose"), true);
ConfigOptionBool optionFerryChecksRelationshipWhenBuying(
    "OpenApoc.NewFeature", "FerryChecksRelationshipWhenBuying",
    tr("Transtellar checks relationship when buying items"), true);
ConfigOptionBool optionAllowManualCityTeleporters("OpenApoc.NewFeature",
                                                  "AllowManualCityTeleporters",
                                                  tr("Allow manual use of teleporters in city"),
                                                  true);
ConfigOptionBool
    optionAllowManualCargoFerry("OpenApoc.NewFeature", "AllowManualCargoFerry",
                                tr("Allow manual ferrying of cargo and non-combatants"), true);
ConfigOptionBool optionAllowSoldierTaxiUse("OpenApoc.NewFeature", "AllowSoldierTaxiUse",
                                           tr("Allow soldiers to call taxi"), true);
ConfigOptionBool optionAllowUnloadingClips("OpenApoc.NewFeature", "AdvancedInventoryControls",
                                           tr("Allow unloading clips and quick equip"), true);
ConfigOptionBool optionPayloadExplosion("OpenApoc.NewFeature", "PayloadExplosion",
                                        tr("Ammunition explodes when blown up"), true);
ConfigOptionBool optionDisplayUnitPaths("OpenApoc.NewFeature", "DisplayUnitPaths",
                                        tr("Display unit paths in battle"), true);
ConfigOptionBool optionAdditionalUnitIcons("OpenApoc.NewFeature", "AdditionalUnitIcons",
                                           tr("Display additional unit icons (fatal, psi)"), true);
ConfigOptionBool optionAllowForceFiringParallel("OpenApoc.NewFeature", "AllowForceFiringParallel",
                                                tr("Allow force-firing parallel to the ground"),
                                                true);
ConfigOptionBool optionRequireLOSToMaintainPsi("OpenApoc.NewFeature", "RequireLOSToMaintainPsi",
                                               tr("Require LOS to maintain psi attack"), false);
ConfigOptionBool optionAllowAttackingOwnedVehicles("OpenApoc.NewFeature",
                                                   "AllowAttackingOwnedVehicles",
                                                   tr("Allow attacking owned vehicles"), true);
ConfigOptionBool optionCallExistingFerry("OpenApoc.NewFeature", "CallExistingFerry",
                                         tr("Call existing transport instead of spawning them"),
                                         true);
ConfigOptionBool
    optionAlternateVehicleShieldSound("OpenApoc.NewFeature", "AlternateVehicleShieldSound",
                                      tr("Hitting vehicle shield produces alternate sound"), true);
ConfigOptionBool optionEnableAgentTemplates("OpenApoc.NewFeature", "EnableAgentTemplates",
                                            tr("Enable agent equipment templates"), true);
ConfigOptionBool
    optionStoreDroppedEquipment("OpenApoc.NewFeature", "StoreDroppedEquipment",
                                tr("Attempt to recover agent equipment dropped in city"), true);
ConfigOptionBool optionFallingGroundVehicles(
    "OpenApoc.NewFeature", "CrashingGroundVehicles",
    tr("Unsupported ground vehicles crash (Weapons and Modules may be lost in crash)"), true);

ConfigOptionBool optionEnforceCargoLimits("OpenApoc.NewFeature", "EnforceCargoLimits",
                                          tr("Enforce vehicle cargo limits"), false);
ConfigOptionBool optionAllowNearbyVehicleLootPickup("OpenApoc.NewFeature",
                                                    "AllowNearbyVehicleLootPickup",
                                                    tr("Allow nearby vehicles to pick up loot"),
                                                    true);
ConfigOptionBool optionAllowBuildingLootDeposit("OpenApoc.NewFeature", "AllowBuildingLootDeposit",
                                                tr("Allow loot to be stashed in the building"),
                                                true);
ConfigOptionBool optionArmoredRoads("OpenApoc.NewFeature", "ArmoredRoads", tr("Armored roads"),
                                    true);
ConfigOptionBool optionVanillaCityControls("OpenApoc.NewFeature", "OpenApocCityControls",
                                           tr("Improved city control scheme"), true);
ConfigOptionBool optionCollapseRaidedBuilding("OpenApoc.NewFeature", "CollapseRaidedBuilding",
                                              tr("Successful raid collapses building"), false);
ConfigOptionBool
    optionScrambleOnUnintentionalHit("OpenApoc.NewFeature", "ScrambleOnUnintentionalHit",
                                     tr("Any hit on hostile building provokes retaliation"), false);
ConfigOptionBool optionMarketRight("OpenApoc.NewFeature", "MarketOnRight",
                                   tr("Put market stock on the right side"), true);
ConfigOptionBool optionDGCrashingVehicles(
    "OpenApoc.NewFeature", "CrashingDimensionGate",
    tr("Uncapable vehicles crash when entering gates (Weapons and Modules may be lost in crash)"),
    true);
ConfigOptionBool optionFuelCrashingVehicles(
    "OpenApoc.NewFeature", "CrashingOutOfFuel",
    tr("Vehicles crash when out of fuel (Weapons and Modules may be lost in crash)"), true);
ConfigOptionBool optionSkipTurbo("OpenApoc.NewFeature", "SkipTurboMovement",
                                 tr("Skip turbo movement calculations"), false);
ConfigOptionBool optionRunAndKneel("OpenApoc.NewFeature", "RunAndKneel",
                                   tr("All units run and kneel by default"), true);
ConfigOptionBool optionSeedRng("OpenApoc.NewFeature", "SeedRng", tr("Seed RNG on game start"),
                               true);
ConfigOptionBool optionAutoReload("OpenApoc.NewFeature", "AutoReload",
                                  tr("Automatically reload weapons when empty"), true);
ConfigOptionBool optionLeftClickIcon("OpenApoc.NewFeature", "LeftClickIconEquip",
                                     tr("Left clicking icon opens equip menu"), false);
ConfigOptionBool optionBattlescapeVertScroll("OpenApoc.NewFeature", "BattlescapeVertScroll",
                                             tr("Mousewheel changes vertical level in battlescape"),
                                             true);
ConfigOptionBool optionSingleSquadSelect("OpenApoc.NewFeature", "SingleSquadSelect",
                                         tr("Select squad with single click"), false);
ConfigOptionBool
    optionATVUFOMission("OpenApoc.NewFeature", "ATVUFOMission",
                        tr("Allow All Terrain Vehicles (ATV) to initiate UFO recovery missions"),
                        true);
ConfigOptionInt
    optionMaxTileRepair("OpenApoc.Mod", "MaxTileRepair",
                        tr("Construction Vehicles will repair a maximum of X Tiles per night"), 5);
ConfigOptionFloat
    optionSceneryRepairCostFactor("OpenApoc.Mod", "SceneryRepairCostFactor",
                                  tr("Determines the percentage of the original Price ORGs have to "
                                     "pay for a Scenery Tile to be repaired"),
                                  10.0f);
ConfigOptionBool optionLoadSameAmmo("OpenApoc.NewFeature", "LoadSameAmmo",
                                    tr("Weapons autoreload only same ammo type"), true);
ConfigOptionBool optionShowCurrentDimensionVehicles(
    "OpenApoc.NewFeature", "ShowCurrentDimensionVehicles",
    tr("Show vehicles in current dimension (or entering / leaving)"), true);
ConfigOptionBool optionShowNonXCOMVehiclesPrefix("OpenApoc.NewFeature", "ShowNonXCOMVehiclesPrefix",
                                                 tr("Add prefix to non-X-COM vehicles"), true);
ConfigOptionBool isoOnlyFollow("OpenApoc.NewFeature", "IsoOnlyFollow",
                               tr("Don't follow vehicles in strategy view"), false);
ConfigOptionBool formatAsCurrency("OpenApoc.NewFeature", "formatAsCurrency",
                                  tr("Use currency formatting"), true);
ConfigOptionBool optionStunHostileAction("OpenApoc.Mod", "StunHostileAction",
                                         tr("Stunning hurts relationships"), false);
ConfigOptionBool optionRaidHostileAction("OpenApoc.Mod", "RaidHostileAction",
                                         tr("Initiating raid hurts relationships"), false);
ConfigOptionBool optionBSKLauncherSound("OpenApoc.Mod", "BSKLauncherSound",
                                        tr("(MOD) Original Brainsucker Launcher SFX"), true);
ConfigOptionBool optionInvulnerableRoads("OpenApoc.Mod", "InvulnerableRoads",
                                         tr("(MOD) Invulnerable roads"), false);
ConfigOptionBool optionATVTank("OpenApoc.Mod", "ATVTank", tr("(MOD) Griffon becomes All-Terrain"),
                               true);

ConfigOptionBool optionATVAPC("OpenApoc.Mod", "ATVAPC",
                              tr("(MOD) Wolfhound APC becomes All-Terrain"), true);

ConfigOptionBool optionCrashingVehicles(
    "OpenApoc.Mod", "CrashingVehicles",
    tr("Vehicles crash on low HP (Weapons and Modules may be lost in crash)"), false);

ConfigOptionString optionScriptsList("OpenApoc.Mod", "ScriptsList",
                                     tr("Semicolon-separated list of scripts to load"),
                                     "scripts/openapoc_base.lua;");

ConfigOptionBool optionInfiniteAmmoCheat("OpenApoc.Cheat", "InfiniteAmmo",
                                         tr("Infinite ammo for X-Com agents and vehicles"), false);
ConfigOptionFloat
    optionDamageInflictedMultiplierCheat("OpenApoc.Cheat", "DamageInflictedMultiplier",
                                         tr("Multiplier for damage inflicted by X-com"), 1.0);
ConfigOptionFloat optionDamageReceivedMultiplierCheat("OpenApoc.Cheat", "DamageReceivedMultiplier",
                                                      tr("Multiplier for damage received by X-com"),
                                                      1.0);
ConfigOptionFloat optionHostilesMultiplierCheat("OpenApoc.Cheat", "HostilesMultiplier",
                                                tr("Multiplier for number of hostiles"), 1.0f);
ConfigOptionFloat optionStatGrowthMultiplierCheat("OpenApoc.Cheat", "StatGrowthMultiplier",
                                                  tr("Multiplier for agent stat growth"), 1.0f);

ConfigOptionBool optionEnableTouchEvents("Framework", "EnableTouchEvents",
                                         tr("Enable touch events"),
#ifdef ANDROID
                                         true
#else
                                         false
#endif
);

ConfigOptionInt imageCacheSize("Framework.Data", "ImageCacheSize",
                               tr("Number of Images to keep in data cache"), 100);
ConfigOptionInt imageSetCacheSize("Framework.Data", "ImageSetCacheSize",
                                  tr("Number of ImageSets to keep in data cache"), 10);
ConfigOptionInt voxelCacheSize("Framework.Data", "VoxelCacheSize",
                               tr("Number of VoxelMaps to keep in data cache"), 1);
ConfigOptionInt fontStringCacheSize("Framework.Data", "FontStringCacheSize",
                                    tr("Number of rendered font stings to keep in data cache"),
                                    100);
ConfigOptionInt paletteCacheSize("Framework.Data", "PaletteCacheSize",
                                 tr("Number of Palettes to keep in data cache"), 10);

ConfigOptionInt fileLogLevelOption(
    "Logger", "FileLevel",
    tr("Loglevel to output to file (0 = nothing, 1 = error, 2 = warning, 3 = info, 4 = debug)"), 3);

ConfigOptionInt
    backtraceLogLevelOption("Logger", "BacktraceLevel",
                            tr("Loglevel to print a backtrace to file log (0 = nothing, 1 "
                               "= error, 2 = warning, 3 = info, 4 = debug)"),
                            1);

ConfigOptionInt dialogLogLevelOption(
    "Logger", "dialogLevel",
    tr("Loglevel to pop up a dialog(0 = nothing, 1 = error, 2 = warning, 3 = info, 4 = debug) "),
    1);

ConfigOptionString defaultTooltipFont("Forms", "TooltipFont", tr("The default tooltip font"),
                                      "smallset");
ConfigOptionBool useCRCChecksum("Framework.Serialization", "CRC",
                                tr("use a CRC checksum when saving files"), false);
ConfigOptionBool useSHA1Checksum("Framework.Serialization", "SHA1",
                                 tr("use a SHA1 checksum when saving files"), false);

ConfigOptionBool enableTrace("Trace", "enable", tr("Enable json call/time tracking"));
ConfigOptionString traceFile("Trace", "outputFile", tr("File to output trace json to"),
                             "openapoc.trace");

ConfigOptionString saveDirOption("Game.Save", "Directory", tr("Directory containing saved games"),
                                 "./saves");
ConfigOptionBool packSaveOption("Game.Save", "Pack", tr("Pack saved games into a zip"), true);

ConfigOptionBool skipIntroOption("Game", "SkipIntro", tr("Skip intro video"), false);
ConfigOptionString loadGameOption("Game", "Load", tr("Path to save game to load at startup"), "");

ConfigOptionString modList("Game", "Mods",
                           tr("A colon-separated list of mods to load (relative to mod directory)"),
                           "base");
ConfigOptionString modPath("Game", "ModPath", tr("Directory containing mods"), "./data/mods");
ConfigOptionBool asyncLoading("Game", "ASyncLoading",
                              tr("Load in background while displaying animated loading screen"),
                              true);

} // namespace OpenApoc::Options
