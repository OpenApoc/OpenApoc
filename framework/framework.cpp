#include "framework/framework.h"
#include "framework/ThreadPool/ThreadPool.h"
#include "framework/apocresources/cursor.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/image.h"
#include "framework/renderer.h"
#include "framework/renderer_interface.h"
#include "framework/sound_interface.h"
#include "framework/stagestack.h"
#include "framework/trace.h"
#include "library/sp.h"
#include <SDL.h>
#include <algorithm>
#include <list>
#include <map>
#include <vector>

// SDL_syswm includes windows.h on windows, which does all kinds of polluting
// defines/namespace stuff, so try to avoid that
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <SDL_syswm.h>

// Windows isn't the only thing that pollutes stuff with #defines - X gets in on it too with 'None'
#undef None

// Use physfs to get prefs dir
#include <physfs.h>

// Boost locale for setting the system locale
// Disable automatic #pragma linking for boost - only enabled in msvc and that should provide boost
// symbols as part of the module that uses it
#define BOOST_ALL_NO_LIB
#include <boost/locale.hpp>

using namespace OpenApoc;

namespace
{

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

ConfigOptionString dataPathOption("Framework", "Data", "The path containing OpenApod data",
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
ConfigOptionBool toolTipsOption("Options.Misc", "ToolTips",
                                "Show tool tips when hovering over controls", true);

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
ConfigOptionBool optionCrashingVehicles("OpenApoc.Mod", "CrashingVehicles",
                                        "Vehicles crash on low HP", false);

} // anonymous namespace

namespace OpenApoc
{

UString Framework::getDataDir() const { return dataPathOption.get(); }

UString Framework::getCDPath() const { return cdPathOption.get(); }

Framework *Framework::instance = nullptr;

class JukeBoxImpl : public JukeBox
{
	Framework &fw;
	unsigned int position;
	std::vector<sp<MusicTrack>> trackList;
	PlayMode mode;

  public:
	JukeBoxImpl(Framework &fw) : fw(fw), mode(PlayMode::Loop) {}
	~JukeBoxImpl() override { this->stop(); }

	void play(std::vector<UString> tracks, PlayMode mode) override
	{
		this->trackList.clear();
		this->position = 0;
		this->mode = mode;
		for (auto &track : tracks)
		{
			auto musicTrack = fw.data->loadMusic(track);
			if (!musicTrack)
				LogError("Failed to load music track \"%s\" - skipping", track);
			else
				this->trackList.push_back(musicTrack);
		}
		this->progressTrack(this);
		this->fw.soundBackend->playMusic(progressTrack, this);
	}
	static void progressTrack(void *data)
	{
		JukeBoxImpl *jukebox = static_cast<JukeBoxImpl *>(data);
		if (jukebox->trackList.size() == 0)
		{
			LogWarning("Trying to play empty jukebox");
			return;
		}
		if (jukebox->position >= jukebox->trackList.size())
		{
			LogInfo("End of jukebox playlist");
			return;
		}
		LogInfo("Playing track %u (%s)", jukebox->position,
		        jukebox->trackList[jukebox->position]->getName());
		jukebox->fw.soundBackend->setTrack(jukebox->trackList[jukebox->position]);

		jukebox->position++;
		if (jukebox->mode == PlayMode::Loop)
			jukebox->position = jukebox->position % jukebox->trackList.size();
	}
	void stop() override { fw.soundBackend->stopMusic(); }
};

class FrameworkPrivate
{
  private:
	friend class Framework;
	bool quitProgram;

	SDL_DisplayMode screenMode;
	SDL_Window *window;
	SDL_GLContext context;

	std::map<UString, std::unique_ptr<RendererFactory>> registeredRenderers;
	std::map<UString, std::unique_ptr<SoundBackendFactory>> registeredSoundBackends;

	std::list<up<Event>> eventQueue;
	std::mutex eventQueueLock;

	StageStack ProgramStages;
	sp<Surface> defaultSurface;
	// The display size may be scaled up to windowSize
	Vec2<int> displaySize;
	Vec2<int> windowSize;

	sp<Surface> scaleSurface;
	up<ThreadPool> threadPool;

	FrameworkPrivate()
	    : quitProgram(false), window(nullptr), context(0), displaySize(0, 0), windowSize(0, 0)
	{
		int threadPoolSize = threadPoolSizeOption.get();
		if (threadPoolSize > 0)
		{
			LogInfo("Set thread pool size to %d", threadPoolSize);
		}
		else if (std::thread::hardware_concurrency() != 0)
		{
			threadPoolSize = std::thread::hardware_concurrency();
			LogInfo("Set thread pool size to reported HW concurrency of %d", threadPoolSize);
		}
		else
		{
			threadPoolSize = 2;
			LogInfo("Failed to get HW concurrency, falling back to pool size %d", threadPoolSize);
		}

		this->threadPool.reset(new ThreadPool(threadPoolSize));
	}
};

Framework::Framework(const UString programName, bool createWindow)
    : p(new FrameworkPrivate), programName(programName), createWindow(createWindow)
{
	TRACE_FN;
	LogInfo("Starting framework");

	if (this->instance)
	{
		LogError("Multiple Framework instances created");
	}

	this->instance = this;

	if (!PHYSFS_isInit())
	{
		if (PHYSFS_init(programName.cStr()) == 0)
		{
			PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
			LogError("Failed to init code %i PHYSFS: %s", (int)error, PHYSFS_getErrorByCode(error));
		}
	}
#ifdef ANDROID
	SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1");
#endif
	// Initialize subsystems separately?
	if (SDL_Init(SDL_INIT_EVENTS) < 0)
	{
		LogError("Cannot init SDL2");
		LogError("SDL error: %s", SDL_GetError());
		p->quitProgram = true;
		return;
	}
	if (createWindow)
	{
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
		{
			LogError("Cannot init SDL_VIDEO - \"%s\"", SDL_GetError());
			p->quitProgram = true;
			return;
		}
	}
	LogInfo("Loading config\n");
	p->quitProgram = false;
	UString settingsPath(PHYSFS_getPrefDir(PROGRAM_ORGANISATION, PROGRAM_NAME));
	settingsPath += "/settings.cfg";

	// This is always set, the default being an empty string (which correctly chooses 'system
	// langauge')
	UString desiredLanguageName;
	if (!languageOption.get().empty())
	{
		desiredLanguageName = languageOption.get();
	}

	LogInfo("Setting up locale \"%s\"", desiredLanguageName);

	boost::locale::generator gen;

	std::vector<UString> resourcePaths;
	resourcePaths.push_back(dataPathOption.get());
	resourcePaths.push_back(cdPathOption.get());

	for (auto &path : resourcePaths)
	{
		auto langPath = path + "/languages";
		LogInfo("Adding \"%s\" to language path", langPath);
		gen.add_messages_path(langPath.str());
	}

	std::vector<UString> translationDomains = {"paedia_string", "ufo_string"};
	for (auto &domain : translationDomains)
	{
		LogInfo("Adding \"%s\" to translation domains", domain);
		gen.add_messages_domain(domain.str());
	}

	std::locale loc = gen(desiredLanguageName.str());
	std::locale::global(loc);

	auto localeName = std::use_facet<boost::locale::info>(loc).name();
	auto localeLang = std::use_facet<boost::locale::info>(loc).language();
	auto localeCountry = std::use_facet<boost::locale::info>(loc).country();
	auto localeVariant = std::use_facet<boost::locale::info>(loc).variant();
	auto localeEncoding = std::use_facet<boost::locale::info>(loc).encoding();
	auto isUTF8 = std::use_facet<boost::locale::info>(loc).utf8();

	LogInfo("Locale info: Name \"%s\" language \"%s\" country \"%s\" variant \"%s\" encoding "
	        "\"%s\" utf8:%s",
	        localeName.c_str(), localeLang.c_str(), localeCountry.c_str(), localeVariant.c_str(),
	        localeEncoding.c_str(), isUTF8 ? "true" : "false");

	this->data.reset(Data::createData(resourcePaths));

	auto testFile = this->data->fs.open("music");
	if (!testFile)
	{
		LogError("Failed to open \"music\" from the CD - likely the cd couldn't be loaded or paths "
		         "are incorrect if using an extracted CD image");
	}

	auto testFile2 = this->data->fs.open("filedoesntexist");
	if (testFile2)
	{
		LogError("Succeded in opening \"FileDoesntExist\" - either you have the weirdest filename "
		         "preferences or something is wrong");
	}
	srand(static_cast<unsigned int>(SDL_GetTicks()));

	if (createWindow)
	{
		displayInitialise();
		audioInitialise();
	}
}

Framework::~Framework()
{
	TRACE_FN;
	LogInfo("Destroying framework");
	// Stop any audio first, as if you've got ongoing music/samples it could call back into the
	// framework for the threadpool/data read/all kinda of stuff it shouldn't do on a half-destroyed
	// framework
	audioShutdown();
	LogInfo("Stopping threadpool");
	p->threadPool.reset();
	LogInfo("Clearing stages");
	p->ProgramStages.clear();
	LogInfo("Saving config");
	if (config().getBool("Config.Save"))
		config().save();

	LogInfo("Shutdown");
	if (createWindow)
	{
		displayShutdown();
	}
	LogInfo("SDL shutdown");
	PHYSFS_deinit();
	SDL_Quit();
	instance = nullptr;
}

Framework &Framework::getInstance()
{
	if (!instance)
	{
		LogError("Framework::getInstance() called with no live Framework");
	}
	return *instance;
}
Framework *Framework::tryGetInstance() { return instance; }

void Framework::run(sp<Stage> initialStage)
{
	size_t frameCount = frameLimit.get();
	if (!createWindow)
	{
		LogError("Trying to run framework without window");
		return;
	}
	size_t frame = 0;
	TRACE_FN;
	LogInfo("Program loop started");

	p->ProgramStages.push(initialStage);

	this->renderer->setPalette(this->data->loadPalette("xcom3/ufodata/pal_06.dat"));

	while (!p->quitProgram)
	{
		frame++;
		TraceObj obj{"Frame", {{"frame", Strings::fromInteger(frame)}}};

		processEvents();

		if (p->ProgramStages.isEmpty())
		{
			break;
		}
		{
			TraceObj updateObj("Update");
			p->ProgramStages.current()->update();
		}

		for (StageCmd cmd : stageCommands)
		{
			switch (cmd.cmd)
			{
				case StageCmd::Command::CONTINUE:
					break;
				case StageCmd::Command::REPLACE:
					p->ProgramStages.pop();
					p->ProgramStages.push(cmd.nextStage);
					break;
				case StageCmd::Command::REPLACEALL:
					p->ProgramStages.clear();
					p->ProgramStages.push(cmd.nextStage);
					break;
				case StageCmd::Command::PUSH:
					p->ProgramStages.push(cmd.nextStage);
					break;
				case StageCmd::Command::POP:
					p->ProgramStages.pop();
					break;
				case StageCmd::Command::QUIT:
					p->quitProgram = true;
					p->ProgramStages.clear();
					break;
			}
			if (p->quitProgram)
			{
				break;
			}
		}
		stageCommands.clear();

		auto surface = p->scaleSurface ? p->scaleSurface : p->defaultSurface;
		RendererSurfaceBinding b(*this->renderer, surface);
		{
			TraceObj objClear{"clear"};
			this->renderer->clear();
		}
		if (!p->ProgramStages.isEmpty())
		{
			TraceObj updateObj("Render");
			p->ProgramStages.current()->render();
			this->cursor->render();
			if (p->scaleSurface)
			{
				RendererSurfaceBinding scaleBind(*this->renderer, p->defaultSurface);
				TraceObj objClear{"clear scale"};
				this->renderer->clear();
				this->renderer->drawScaled(p->scaleSurface, {0, 0}, p->windowSize);
			}
			{
				TraceObj flipObj("Flip");
				this->renderer->flush();
				this->renderer->newFrame();
				SDL_GL_SwapWindow(p->window);
			}
		}
		if (frameCount && frame == frameCount)
		{
			LogWarning("Quitting hitting frame count limit of %llu", (unsigned long long)frame);
			p->quitProgram = true;
		}
	}
}

void Framework::processEvents()
{
	TRACE_FN;
	if (p->ProgramStages.isEmpty())
	{
		p->quitProgram = true;
		return;
	}

	// TODO: Consider threading the translation
	translateSdlEvents();

	while (p->eventQueue.size() > 0 && !p->ProgramStages.isEmpty())
	{
		up<Event> e;
		{
			std::lock_guard<std::mutex> l(p->eventQueueLock);
			e = std::move(p->eventQueue.front());
			p->eventQueue.pop_front();
		}
		if (!e)
		{
			LogError("Invalid event on queue");
			continue;
		}
		this->cursor->eventOccured(e.get());
		if (e->type() == EVENT_KEY_DOWN)
		{
			if (e->keyboard().KeyCode == SDLK_PRINTSCREEN)
			{
				UString screenshotName = "screenshot.png";
				LogWarning("Writing screenshot to \"%s\"", screenshotName);
				if (!p->defaultSurface->rendererPrivateData)
				{
					LogWarning("No renderer data on surface - nothing drawn yet?");
				}
				else
				{
					auto img = p->defaultSurface->rendererPrivateData->readBack();
					if (!img)
					{
						LogWarning("No image returned");
					}
					else
					{
						auto ret = data->writeImage(screenshotName, img);
						if (!ret)
						{
							LogWarning("Failed to write screenshot");
						}
						else
						{
							LogWarning("Wrote screenshot to \"%s\"", screenshotName);
						}
					}
				}
			}
		}
		switch (e->type())
		{
			case EVENT_WINDOW_CLOSED:
				shutdownFramework();
				return;
			default:
				p->ProgramStages.current()->eventOccurred(e.get());
				break;
		}
	}
	/* Drop any events left in the list, as it's possible an event caused the last stage to pop with
	 * events outstanding, but they can safely be ignored as we're quitting anyway */
	{
		std::lock_guard<std::mutex> l(p->eventQueueLock);
		p->eventQueue.clear();
	}
}

void Framework::pushEvent(up<Event> e)
{
	std::lock_guard<std::mutex> l(p->eventQueueLock);
	p->eventQueue.push_back(std::move(e));
}

void Framework::pushEvent(Event *e) { this->pushEvent(up<Event>(e)); }

void Framework::translateSdlEvents()
{
	SDL_Event e;
	Event *fwE;

	// FIXME: That's not the right way to figure out the primary finger!
	int primaryFingerID = -1;
	if (SDL_GetNumTouchDevices())
	{
		SDL_Finger *primaryFinger = SDL_GetTouchFinger(SDL_GetTouchDevice(0), 0);
		if (primaryFinger)
		{
			primaryFingerID = primaryFinger->id;
		}
	}

	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
			case SDL_QUIT:
				fwE = new DisplayEvent(EVENT_WINDOW_CLOSED);
				pushEvent(up<Event>(fwE));
				break;
			case SDL_JOYDEVICEADDED:
			case SDL_JOYDEVICEREMOVED:
				// FIXME: Do nothing?
				break;
			case SDL_KEYDOWN:
				fwE = new KeyboardEvent(EVENT_KEY_DOWN);
				fwE->keyboard().KeyCode = e.key.keysym.sym;
				fwE->keyboard().ScanCode = e.key.keysym.scancode;
				fwE->keyboard().Modifiers = e.key.keysym.mod;
				pushEvent(up<Event>(fwE));
				break;
			case SDL_KEYUP:
				fwE = new KeyboardEvent(EVENT_KEY_UP);
				fwE->keyboard().KeyCode = e.key.keysym.sym;
				fwE->keyboard().ScanCode = e.key.keysym.scancode;
				fwE->keyboard().Modifiers = e.key.keysym.mod;
				pushEvent(up<Event>(fwE));
				break;
			case SDL_TEXTINPUT:
				fwE = new TextEvent();
				fwE->text().Input = e.text.text;
				pushEvent(up<Event>(fwE));
				break;
			case SDL_TEXTEDITING:
				// FIXME: Do nothing?
				break;
			case SDL_MOUSEMOTION:
				fwE = new MouseEvent(EVENT_MOUSE_MOVE);
				fwE->mouse().X = e.motion.x;
				fwE->mouse().Y = e.motion.y;
				fwE->mouse().DeltaX = e.motion.xrel;
				fwE->mouse().DeltaY = e.motion.yrel;
				fwE->mouse().WheelVertical = 0;   // These should be handled
				fwE->mouse().WheelHorizontal = 0; // in a separate event
				fwE->mouse().Button = e.motion.state;
				pushEvent(up<Event>(fwE));
				break;
			case SDL_MOUSEWHEEL:
				// FIXME: Check these values for sanity
				fwE = new MouseEvent(EVENT_MOUSE_MOVE);
				// Since I'm using some variables that are not used anywhere else,
				// this code should be in its own small block.
				{
					int mx, my;
					fwE->mouse().Button = SDL_GetMouseState(&mx, &my);
					fwE->mouse().X = mx;
					fwE->mouse().Y = my;
					fwE->mouse().DeltaX = 0; // FIXME: This might cause problems?
					fwE->mouse().DeltaY = 0;
					fwE->mouse().WheelVertical = e.wheel.y;
					fwE->mouse().WheelHorizontal = e.wheel.x;
				}
				pushEvent(up<Event>(fwE));
				break;
			case SDL_MOUSEBUTTONDOWN:
				fwE = new MouseEvent(EVENT_MOUSE_DOWN);
				fwE->mouse().X = e.button.x;
				fwE->mouse().Y = e.button.y;
				fwE->mouse().DeltaX = 0; // FIXME: This might cause problems?
				fwE->mouse().DeltaY = 0;
				fwE->mouse().WheelVertical = 0;
				fwE->mouse().WheelHorizontal = 0;
				fwE->mouse().Button = SDL_BUTTON(e.button.button);
				pushEvent(up<Event>(fwE));
				break;
			case SDL_MOUSEBUTTONUP:
				fwE = new MouseEvent(EVENT_MOUSE_UP);
				fwE->mouse().X = e.button.x;
				fwE->mouse().Y = e.button.y;
				fwE->mouse().DeltaX = 0; // FIXME: This might cause problems?
				fwE->mouse().DeltaY = 0;
				fwE->mouse().WheelVertical = 0;
				fwE->mouse().WheelHorizontal = 0;
				fwE->mouse().Button = SDL_BUTTON(e.button.button);
				pushEvent(up<Event>(fwE));
				break;
			case SDL_FINGERDOWN:
				fwE = new FingerEvent(EVENT_FINGER_DOWN);
				fwE->finger().X = static_cast<int>(e.tfinger.x * displayGetWidth());
				fwE->finger().Y = static_cast<int>(e.tfinger.y * displayGetHeight());
				fwE->finger().DeltaX = static_cast<int>(e.tfinger.dx * displayGetWidth());
				fwE->finger().DeltaY = static_cast<int>(e.tfinger.dy * displayGetHeight());
				fwE->finger().Id = e.tfinger.fingerId;
				fwE->finger().IsPrimary =
				    e.tfinger.fingerId ==
				    primaryFingerID; // FIXME: Try to remember the ID of the first touching finger!
				pushEvent(up<Event>(fwE));
				break;
			case SDL_FINGERUP:
				fwE = new FingerEvent(EVENT_FINGER_UP);
				fwE->finger().X = static_cast<int>(e.tfinger.x * displayGetWidth());
				fwE->finger().Y = static_cast<int>(e.tfinger.y * displayGetHeight());
				fwE->finger().DeltaX = static_cast<int>(e.tfinger.dx * displayGetWidth());
				fwE->finger().DeltaY = static_cast<int>(e.tfinger.dy * displayGetHeight());
				fwE->finger().Id = e.tfinger.fingerId;
				fwE->finger().IsPrimary =
				    e.tfinger.fingerId ==
				    primaryFingerID; // FIXME: Try to remember the ID of the first touching finger!
				pushEvent(up<Event>(fwE));
				break;
			case SDL_FINGERMOTION:
				fwE = new FingerEvent(EVENT_FINGER_MOVE);
				fwE->finger().X = static_cast<int>(e.tfinger.x * displayGetWidth());
				fwE->finger().Y = static_cast<int>(e.tfinger.y * displayGetHeight());
				fwE->finger().DeltaX = static_cast<int>(e.tfinger.dx * displayGetWidth());
				fwE->finger().DeltaY = static_cast<int>(e.tfinger.dy * displayGetHeight());
				fwE->finger().Id = e.tfinger.fingerId;
				fwE->finger().IsPrimary =
				    e.tfinger.fingerId ==
				    primaryFingerID; // FIXME: Try to remember the ID of the first touching finger!
				pushEvent(up<Event>(fwE));
				break;
			case SDL_WINDOWEVENT:
				// Window events get special treatment
				switch (e.window.event)
				{
					case SDL_WINDOWEVENT_RESIZED:
						// FIXME: Do we care about SDL_WINDOWEVENT_SIZE_CHANGED?
						fwE = new DisplayEvent(EVENT_WINDOW_RESIZE);
						fwE->display().X = 0;
						fwE->display().Y = 0;
						fwE->display().Width = e.window.data1;
						fwE->display().Height = e.window.data2;
						fwE->display().Active = true;
						pushEvent(up<Event>(fwE));
						break;
					case SDL_WINDOWEVENT_HIDDEN:
					case SDL_WINDOWEVENT_MINIMIZED:
					case SDL_WINDOWEVENT_LEAVE:
						// FIXME: Check if we should react this way for each of those events
						// FIXME: Check if we're missing some of the events
						fwE = new DisplayEvent(EVENT_WINDOW_DEACTIVATE);
						fwE->display().X = 0;
						fwE->display().Y = 0;
						// FIXME: Is this even necessary?
						SDL_GetWindowSize(p->window, &(fwE->display().Width),
						                  &(fwE->display().Height));
						fwE->display().Active = false;
						pushEvent(up<Event>(fwE));
						break;
					case SDL_WINDOWEVENT_SHOWN:
					case SDL_WINDOWEVENT_EXPOSED:
					case SDL_WINDOWEVENT_RESTORED:
					case SDL_WINDOWEVENT_ENTER:
						// FIXME: Should we handle all these events as "aaand we're back" events?
						fwE = new DisplayEvent(EVENT_WINDOW_ACTIVATE);
						fwE->display().X = 0;
						fwE->display().Y = 0;
						// FIXME: Is this even necessary?
						SDL_GetWindowSize(p->window, &(fwE->display().Width),
						                  &(fwE->display().Height));
						fwE->display().Active = false;
						pushEvent(up<Event>(fwE));
						break;
					case SDL_WINDOWEVENT_CLOSE:
						// Closing a window will be a "quit" event.
						e.type = SDL_QUIT;
						SDL_PushEvent(&e);
						break;
				}
				break;
			default:
				break;
		}
	}
}

void Framework::shutdownFramework()
{
	LogInfo("Shutdown framework");
	p->ProgramStages.clear();
	p->quitProgram = true;
}

void Framework::displayInitialise()
{
	if (!this->createWindow)
	{
		return;
	}
	TRACE_FN;
	LogInfo("Init display");
	int display_flags = SDL_WINDOW_OPENGL;
#ifdef OPENAPOC_GLES
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
#ifdef SDL_OPENGL_CORE
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

#endif
#ifdef DEBUG_RENDERER
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
	// Request context version 3.0
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	// Request RGBA8888 - change if needed
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GL_SetSwapInterval(swapInterval.get());

	int scrW = screenWidthOption.get();
	int scrH = screenHeightOption.get();
	bool scrFS = screenFullscreenOption.get();

	if (scrW < 640 || scrH < 480)
	{
		LogError(
		    "Requested display size of {%d,%d} is lower than {640,480} and probably won't work",
		    scrW, scrH);
	}

	if (scrFS)
	{
		display_flags |= SDL_WINDOW_FULLSCREEN;
	}

	p->window = SDL_CreateWindow("OpenApoc", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, scrW,
	                             scrH, display_flags);

	if (!p->window)
	{
		LogError("Failed to create window \"%s\"", SDL_GetError());
		exit(1);
	}

	p->context = SDL_GL_CreateContext(p->window);
	if (!p->context)
	{
		LogWarning("Could not create GL context! [SDLError: %s]", SDL_GetError());
		LogWarning("Attempting to create context by lowering the requested version");
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		p->context = SDL_GL_CreateContext(p->window);
		if (!p->context)
		{
			LogError("Failed to create GL context! [SDLerror: %s]", SDL_GetError());
			SDL_DestroyWindow(p->window);
			exit(1);
		}
	}
	// Output the context parameters
	LogInfo("Created OpenGL context, parameters:");
	int value;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &value);
	UString profileType;
	switch (value)
	{
		case SDL_GL_CONTEXT_PROFILE_ES:
			profileType = "ES";
			break;
		case SDL_GL_CONTEXT_PROFILE_CORE:
			profileType = "Core";
			break;
		case SDL_GL_CONTEXT_PROFILE_COMPATIBILITY:
			profileType = "Compatibility";
			break;
		default:
			profileType = "Unknown";
	}
	LogInfo("  Context profile: %s", profileType);
	int ctxMajor, ctxMinor;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &ctxMajor);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &ctxMinor);
	LogInfo("  Context version: %d.%d", ctxMajor, ctxMinor);
	int bitsRed, bitsGreen, bitsBlue, bitsAlpha;
	SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &bitsRed);
	SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &bitsGreen);
	SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &bitsBlue);
	SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &bitsAlpha);
	LogInfo("  RGBA bits: %d-%d-%d-%d", bitsRed, bitsGreen, bitsBlue, bitsAlpha);
	SDL_GL_MakeCurrent(p->window, p->context); // for good measure?
	SDL_ShowCursor(SDL_DISABLE);

	p->registeredRenderers["GLES_3_0"].reset(getGLES30RendererFactory());
#ifndef __ANDROID__ // GL2 is not available on Android
	p->registeredRenderers["GL_2_0"].reset(getGL20RendererFactory());
#endif

	for (auto &rendererName : renderersOption.get().split(':'))
	{
		auto rendererFactory = p->registeredRenderers.find(rendererName);
		if (rendererFactory == p->registeredRenderers.end())
		{
			LogInfo("Renderer \"%s\" not in supported list", rendererName);
			continue;
		}
		Renderer *r = rendererFactory->second->create();
		if (!r)
		{
			LogInfo("Renderer \"%s\" failed to init", rendererName);
			continue;
		}
		this->renderer.reset(r);
		LogInfo("Using renderer: %s", this->renderer->getName());
		break;
	}
	if (!this->renderer)
	{
		LogError("No functional renderer found");
		abort();
	}
	this->p->defaultSurface = this->renderer->getDefaultSurface();

	int width, height;
	SDL_GetWindowSize(p->window, &width, &height);
	p->windowSize = {width, height};

	// FIXME: Scale is currently stored as an integer in 1/100 units (ie 100 is 1.0 == same size)
	int scaleX = screenScaleXOption.get();
	int scaleY = screenScaleYOption.get();

	if (scaleX != 100 || scaleY != 100)
	{
		float scaleXFloat = (float)scaleX / 100.0f;
		float scaleYFloat = (float)scaleY / 100.0f;
		p->displaySize.x = (int)((float)p->windowSize.x * scaleXFloat);
		p->displaySize.y = (int)((float)p->windowSize.y * scaleYFloat);
		if (p->displaySize.x < 640 || p->displaySize.y < 480)
		{
			LogWarning("Requested scaled size of %s is lower than {640,480} and probably "
			           "won't work, so forcing 640x480",
			           p->displaySize.x);
			p->displaySize.x = std::max(640, p->displaySize.x);
			p->displaySize.y = std::max(480, p->displaySize.y);
		}
		LogInfo("Scaling from %s to %s", p->displaySize, p->windowSize);
		p->scaleSurface = mksp<Surface>(p->displaySize);
	}
	else
	{
		p->displaySize = p->windowSize;
	}
	this->cursor.reset(new ApocCursor(this->data->loadPalette("xcom3/tacdata/tactical.pal")));
}

void Framework::displayShutdown()
{
	this->cursor.reset();
	if (!p->window)
	{
		return;
	}
	TRACE_FN;
	LogInfo("Shutdown Display");
	p->defaultSurface.reset();
	renderer.reset();

	SDL_GL_DeleteContext(p->context);
	SDL_DestroyWindow(p->window);
}

int Framework::displayGetWidth() { return p->displaySize.x; }

int Framework::displayGetHeight() { return p->displaySize.y; }

Vec2<int> Framework::displayGetSize() { return p->displaySize; }

bool Framework::displayHasWindow() const
{
	if (createWindow == false)
		return false;
	if (!p->window)
		return false;
	return true;
}

void Framework::displaySetTitle(UString NewTitle)
{
	if (p->window)
	{
		SDL_SetWindowTitle(p->window, NewTitle.cStr());
	}
}

void Framework::displaySetIcon()
{
	if (!p->window)
	{
		return;
	}
#ifdef _WIN32
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	SDL_GetWindowWMInfo(p->window, &info);
	HINSTANCE handle = GetModuleHandle(NULL);
	HICON icon = LoadIcon(handle, L"ALLEGRO_ICON");
	HWND hwnd = info.info.win.window;
	SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR)icon);
#else
// TODO: Figure out how this works
// NOTE: this should be a call to SDL_SetWindowIcon(p->window, icon);
// where icon should be a SDL_Surface* with our desired icon.
#endif
}

void Framework::audioInitialise()
{
	TRACE_FN;
	LogInfo("Initialise Audio");

	p->registeredSoundBackends["SDLRaw"].reset(getSDLSoundBackend());
	p->registeredSoundBackends["null"].reset(getNullSoundBackend());

	for (auto &soundBackendName : audioBackendsOption.get().split(':'))
	{
		auto backendFactory = p->registeredSoundBackends.find(soundBackendName);
		if (backendFactory == p->registeredSoundBackends.end())
		{
			LogInfo("Sound backend %s not in supported list", soundBackendName);
			continue;
		}
		SoundBackend *backend = backendFactory->second->create();
		if (!backend)
		{
			LogInfo("Sound backend %s failed to init", soundBackendName);
			continue;
		}
		this->soundBackend.reset(backend);
		LogInfo("Using sound backend %s", soundBackendName);
		break;
	}
	if (!this->soundBackend)
	{
		LogError("No functional sound backend found");
	}
	this->jukebox.reset(new JukeBoxImpl(*this));

	/* Setup initial gain */
	this->soundBackend->setGain(SoundBackend::Gain::Global,
	                            static_cast<float>(audioGlobalGainOption.get()) / 20.0f);
	this->soundBackend->setGain(SoundBackend::Gain::Music,
	                            static_cast<float>(audioMusicGainOption.get()) / 20.0f);
	this->soundBackend->setGain(SoundBackend::Gain::Sample,
	                            static_cast<float>(audioSampleGainOption.get()) / 20.0f);
}

void Framework::audioShutdown()
{
	TRACE_FN;
	LogInfo("Shutdown Audio");
	this->jukebox.reset();
	this->soundBackend.reset();
}

sp<Stage> Framework::stageGetCurrent() { return p->ProgramStages.current(); }

sp<Stage> Framework::stageGetPrevious() { return p->ProgramStages.previous(); }

sp<Stage> Framework::stageGetPrevious(sp<Stage> From) { return p->ProgramStages.previous(From); }

void Framework::stageQueueCommand(const StageCmd &cmd) { stageCommands.emplace_back(cmd); }

ApocCursor &Framework::getCursor() { return *this->cursor; }

void Framework::textStartInput() { SDL_StartTextInput(); }

void Framework::textStopInput() { SDL_StopTextInput(); }

UString Framework::textGetClipboard()
{
	UString str;
	char *text = SDL_GetClipboardText();
	if (text != nullptr)
	{
		str = text;
		SDL_free(text);
	}
	return str;
}

void Framework::threadPoolTaskEnqueue(std::function<void()> task) { p->threadPool->enqueue(task); }

}; // namespace OpenApoc