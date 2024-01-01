#include "game/ui/general/moreoptions.h"
#include "forms/checkbox.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/scrollbar.h"
#include "forms/textbutton.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"
#include "game/state/shared/organisation.h"
#include <forms/graphic.h>
#include <limits>

namespace OpenApoc
{
namespace
{
std::list<std::pair<UString, UString>> cityscapeList = {
    {"OpenApoc.NewFeature", "FerryChecksRelationshipWhenBuying"},
    {"OpenApoc.NewFeature", "AllowManualCityTeleporters"},
    {"OpenApoc.NewFeature", "AllowManualCargoFerry"},
    {"OpenApoc.NewFeature", "AllowSoldierTaxiUse"},
    {"OpenApoc.NewFeature", "AllowAttackingOwnedVehicles"},
    {"OpenApoc.NewFeature", "CallExistingFerry"},
    {"OpenApoc.NewFeature", "StoreDroppedEquipment"},
    {"OpenApoc.NewFeature", "EnforceCargoLimits"},
    {"OpenApoc.NewFeature", "AllowNearbyVehicleLootPickup"},
    {"OpenApoc.NewFeature", "AllowBuildingLootDeposit"},
    {"OpenApoc.NewFeature", "ArmoredRoads"},
    {"OpenApoc.NewFeature", "CrashingGroundVehicles"},
    {"OpenApoc.NewFeature", "OpenApocCityControls"},
    {"OpenApoc.NewFeature", "CollapseRaidedBuilding"},
    {"OpenApoc.NewFeature", "ScrambleOnUnintentionalHit"},
    {"OpenApoc.NewFeature", "MarketOnRight"},
    {"OpenApoc.NewFeature", "LeftClickIconEquip"},
    {"OpenApoc.NewFeature", "CrashingDimensionGate"},
    {"OpenApoc.NewFeature", "SkipTurboMovement"},
    {"OpenApoc.NewFeature", "CrashingOutOfFuel"},
    {"OpenApoc.NewFeature", "ATVUFOMission"},
    {"OpenApoc.NewFeature", "RepairWithConstructionVehicles"},
    {"OpenApoc.Mod", "RaidHostileAction"},
    {"OpenApoc.Mod", "CrashingVehicles"},
    {"OpenApoc.Mod", "InvulnerableRoads"},
    {"OpenApoc.Mod", "ATVTank"},
    {"OpenApoc.Mod", "ATVAPC"},
};

std::list<std::pair<UString, UString>> battlescapeList = {
    {"OpenApoc.NewFeature", "InstantExplosionDamage"},
    {"OpenApoc.NewFeature", "UFODamageModel"},
    {"OpenApoc.NewFeature", "GravliftSounds"},
    {"OpenApoc.NewFeature", "NoInstantThrows"},
    {"OpenApoc.NewFeature", "PayloadExplosion"},
    {"OpenApoc.NewFeature", "DisplayUnitPaths"},
    {"OpenApoc.NewFeature", "AllowForceFiringParallel"},
    {"OpenApoc.NewFeature", "RequireLOSToMaintainPsi"},
    {"OpenApoc.NewFeature", "AlternateVehicleShieldSound"},
    {"OpenApoc.NewFeature", "RunAndKneel"},
    {"OpenApoc.NewFeature", "AutoReload"},
    {"OpenApoc.NewFeature", "BattlescapeVertScroll"},
    {"OpenApoc.NewFeature", "SingleSquadSelect"},
    {"OpenApoc.Mod", "StunHostileAction"},
    {"OpenApoc.Mod", "BSKLauncherSound"},
};

// TODO: Implement vanilla mode
std::list<std::pair<UString, UString>> vanillaList = {
    {"OpenApoc.NewFeature", "PayloadExplosion"},
    {"OpenApoc.NewFeature", "DisplayUnitPaths"},
    {"OpenApoc.NewFeature", "AdditionalUnitIcons"},
    {"OpenApoc.NewFeature", "AllowForceFiringParallel"},
    {"OpenApoc.NewFeature", "FerryChecksRelationshipWhenBuying"},
    {"OpenApoc.NewFeature", "AllowSoldierTaxiUse"},
    {"OpenApoc.NewFeature", "AllowAttackingOwnedVehicles"},
    {"OpenApoc.NewFeature", "StoreDroppedEquipment"},
    {"OpenApoc.NewFeature", "EnforceCargoLimits"},
    {"OpenApoc.NewFeature", "SeedRng"},
    {"OpenApoc.NewFeature", "AutoReload"},
    {"OpenApoc.Mod", "RaidHostileAction"},
};

} // namespace
MoreOptions::MoreOptions(sp<GameState> state)
    : Stage(), menuform(ui().getForm("moreoptions")), state(state)
{
}
MoreOptions::~MoreOptions() {}

void MoreOptions::saveLists()
{
	auto citylistControl = menuform->findControlTyped<ListBox>("CITY_NOTIFICATIONS_LIST");
	for (auto &c : citylistControl->Controls)
	{
		auto name = c->getData<UString>();
		config().set(*name, std::dynamic_pointer_cast<CheckBox>(c)->isChecked());
	}

	auto battlelistControl = menuform->findControlTyped<ListBox>("BATTLE_NOTIFICATIONS_LIST");
	for (auto &b : battlelistControl->Controls)
	{
		auto name = b->getData<UString>();
		config().set(*name, std::dynamic_pointer_cast<CheckBox>(b)->isChecked());
	}
}

void MoreOptions::loadLists()
{
	saveLists();
	menuform->findControlTyped<Label>("CITYLIST_NAME")->setText("Cityscape Options");
	menuform->findControlTyped<Label>("BATTLELIST_NAME")->setText("Battlescape Options");
	std::list<std::pair<UString, UString>> *citynotificationList = nullptr;
	std::list<std::pair<UString, UString>> *battlenotificationList = nullptr;

	citynotificationList = &cityscapeList;
	battlenotificationList = &battlescapeList;

	auto citylistControl = menuform->findControlTyped<ListBox>("CITY_NOTIFICATIONS_LIST");
	auto battlelistControl = menuform->findControlTyped<ListBox>("BATTLE_NOTIFICATIONS_LIST");
	citylistControl->clear();
	battlelistControl->clear();
	auto font = ui().getFont("smalfont");

	// FIXME: Can this be optimized?
	for (auto &p : *citynotificationList)
	{
		auto checkBox = mksp<CheckBox>(fw().data->loadImage("BUTTON_CHECKBOX_TRUE"),
		                               fw().data->loadImage("BUTTON_CHECKBOX_FALSE"));
		checkBox->Size = {240, citylistControl->ItemSize};
		UString full_name = p.first + "." + p.second;
		checkBox->setData(mksp<UString>(full_name));
		checkBox->setChecked(config().getBool(full_name));
		auto label = checkBox->createChild<Label>(tr(config().describe(p.first, p.second)), font);
		label->Size = {216, citylistControl->ItemSize};
		label->Location = {24, 0};
		label->ToolTipText = tr(config().describe(p.first, p.second));
		label->ToolTipFont = font;
		citylistControl->addItem(checkBox);
	}

	for (auto &p : *battlenotificationList)
	{
		auto checkBox = mksp<CheckBox>(fw().data->loadImage("BUTTON_CHECKBOX_TRUE"),
		                               fw().data->loadImage("BUTTON_CHECKBOX_FALSE"));
		checkBox->Size = {240, battlelistControl->ItemSize};
		UString full_name = p.first + "." + p.second;
		checkBox->setData(mksp<UString>(full_name));
		checkBox->setChecked(config().getBool(full_name));
		auto label = checkBox->createChild<Label>(tr(config().describe(p.first, p.second)), font);
		label->Size = {216, battlelistControl->ItemSize};
		label->Location = {24, 0};
		label->ToolTipText = tr(config().describe(p.first, p.second));
		label->ToolTipFont = font;
		battlelistControl->addItem(checkBox);
	}
}

bool MoreOptions::isTransition() { return false; }

void MoreOptions::begin()
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	loadLists();

	// TODO: Implement vanilla mode
	// menuform->findControlTyped<CheckBox>("VANILLA_TOGGLE")
	//    ->setChecked(config().getBool("Options.Misc.VanillaToggle"));
	menuform->findControlTyped<CheckBox>("DEBUGVIS_TOGGLE")
	    ->setChecked(config().getBool("OpenApoc.NewFeature.DebugCommandsVisible"));
	menuform->findControlTyped<CheckBox>("SCROLLSOUND_TOGGLE")
	    ->setChecked(config().getBool("OpenApoc.NewFeature.NoScrollSounds"));
	menuform->findControlTyped<CheckBox>("MOREICONS_TOGGLE")
	    ->setChecked(config().getBool("OpenApoc.NewFeature.AdditionalUnitIcons"));
	menuform->findControlTyped<CheckBox>("ADVINVENTORY_TOGGLE")
	    ->setChecked(config().getBool("OpenApoc.NewFeature.AdvancedInventoryControls"));
	menuform->findControlTyped<CheckBox>("TEMPLATES_TOGGLE")
	    ->setChecked(config().getBool("OpenApoc.NewFeature.EnableAgentTemplates"));
	menuform->findControlTyped<CheckBox>("SEEDRNG_TOGGLE")
	    ->setChecked(config().getBool("OpenApoc.NewFeature.SeedRng"));
}

void MoreOptions::pause() {}

void MoreOptions::resume() {}

void MoreOptions::finish()
{
	// TODO: Implement vanilla mode
	// config().set("Options.Misc.VanillaToggle",
	//             menuform->findControlTyped<CheckBox>("VANILLA_TOGGLE")->isChecked());
	config().set("OpenApoc.NewFeature.DebugCommandsVisible",
	             menuform->findControlTyped<CheckBox>("DEBUGVIS_TOGGLE")->isChecked());
	config().set("OpenApoc.NewFeature.NoScrollSounds",
	             menuform->findControlTyped<CheckBox>("SCROLLSOUND_TOGGLE")->isChecked());
	config().set("OpenApoc.NewFeature.AdditionalUnitIcons",
	             menuform->findControlTyped<CheckBox>("MOREICONS_TOGGLE")->isChecked());
	config().set("OpenApoc.NewFeature.AdvancedInventoryControls",
	             menuform->findControlTyped<CheckBox>("ADVINVENTORY_TOGGLE")->isChecked());
	config().set("OpenApoc.NewFeature.EnableAgentTemplates",
	             menuform->findControlTyped<CheckBox>("TEMPLATES_TOGGLE")->isChecked());
	config().set("OpenApoc.NewFeature.SeedRng",
	             menuform->findControlTyped<CheckBox>("SEEDRNG_TOGGLE")->isChecked());
	saveLists();
}

void MoreOptions::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_RETURN ||
		    e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_OK")->click();
			return;
		}
	}
	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_OK")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}
}

void MoreOptions::update() { menuform->update(); }

void MoreOptions::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}
} // namespace OpenApoc
