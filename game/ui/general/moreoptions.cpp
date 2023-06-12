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
#include <limits>

namespace OpenApoc
{
namespace
{
std::list<std::pair<UString, UString>> openApocList = {
    {"OpenApoc.NewFeature", "DebugCommandsVisible"},
    {"OpenApoc.NewFeature", "UFODamageModel"},
    {"OpenApoc.NewFeature", "InstantExplosionDamage"},
    {"OpenApoc.NewFeature", "GravliftSounds"},
    {"OpenApoc.NewFeature", "NoScrollSounds"},
    {"OpenApoc.NewFeature", "NoInstantThrows"},
    {"OpenApoc.NewFeature", "PayloadExplosion"},
    {"OpenApoc.NewFeature", "DisplayUnitPaths"},
    {"OpenApoc.NewFeature", "AdditionalUnitIcons"},
    {"OpenApoc.NewFeature", "AllowForceFiringParallel"},
    {"OpenApoc.NewFeature", "RequireLOSToMaintainPsi"},
    {"OpenApoc.NewFeature", "AdvancedInventoryControls"},
    {"OpenApoc.NewFeature", "EnableAgentTemplates"},
    {"OpenApoc.NewFeature", "FerryChecksRelationshipWhenBuying"},
    {"OpenApoc.NewFeature", "AllowManualCityTeleporters"},
    {"OpenApoc.NewFeature", "AllowManualCargoFerry"},
    {"OpenApoc.NewFeature", "AllowSoldierTaxiUse"},
    {"OpenApoc.NewFeature", "AllowAttackingOwnedVehicles"},
    {"OpenApoc.NewFeature", "CallExistingFerry"},
    {"OpenApoc.NewFeature", "AlternateVehicleShieldSound"},
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
    {"OpenApoc.NewFeature", "CrashingDimensionGate"},
    {"OpenApoc.NewFeature", "SkipTurboMovement"},
    {"OpenApoc.NewFeature", "CrashingOutOfFuel"},
    {"OpenApoc.NewFeature", "RunAndKneel"},
    {"OpenApoc.NewFeature", "SeedRng"},
    {"OpenApoc.NewFeature", "AutoReload"},
    {"OpenApoc.NewFeature", "LeftClickIconEquip"},
    {"OpenApoc.NewFeature", "BattlescapeVertScroll"},
    {"OpenApoc.NewFeature", "SingleSquadSelect"},

    {"OpenApoc.Mod", "StunHostileAction"},
    {"OpenApoc.Mod", "RaidHostileAction"},
    {"OpenApoc.Mod", "CrashingVehicles"},
    {"OpenApoc.Mod", "InvulnerableRoads"},
    {"OpenApoc.Mod", "ATVTank"},
    {"OpenApoc.Mod", "ATVAPC"},
    {"OpenApoc.Mod", "BSKLauncherSound"},
};

} // namespace
MoreOptions::MoreOptions(sp<GameState> state)
    : Stage(), menuform(ui().getForm("moreoptions")), state(state)
{
}
MoreOptions::~MoreOptions() {}

void MoreOptions::saveList()
{
	auto listControl = menuform->findControlTyped<ListBox>("NOTIFICATIONS_LIST");
	for (auto &c : listControl->Controls)
	{
		auto name = c->getData<UString>();
		config().set(*name, std::dynamic_pointer_cast<CheckBox>(c)->isChecked());
	}
}

void MoreOptions::loadList()
{
	saveList();
	menuform->findControlTyped<Label>("LIST_NAME");
	std::list<std::pair<UString, UString>> *notificationList = nullptr;

	notificationList = &openApocList;

	auto listControl = menuform->findControlTyped<ListBox>("NOTIFICATIONS_LIST");
	listControl->clear();
	auto font = ui().getFont("smalfont");
	for (auto &p : *notificationList)
	{
		auto checkBox = mksp<CheckBox>(fw().data->loadImage("BUTTON_CHECKBOX_TRUE"),
		                               fw().data->loadImage("BUTTON_CHECKBOX_FALSE"));
		checkBox->Size = {240, listControl->ItemSize};
		UString full_name = p.first + "." + p.second;
		checkBox->setData(mksp<UString>(full_name));
		checkBox->setChecked(config().getBool(full_name));
		auto label = checkBox->createChild<Label>(tr(config().describe(p.first, p.second)), font);
		label->Size = {216, listControl->ItemSize};
		label->Location = {24, 0};
		label->ToolTipText = tr(config().describe(p.first, p.second));
		label->ToolTipFont = font;
		listControl->addItem(checkBox);
	}
}

bool MoreOptions::isTransition() { return false; }

void MoreOptions::begin()
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	loadList();
}

void MoreOptions::pause() {}

void MoreOptions::resume() {}

void MoreOptions::finish() {}

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
