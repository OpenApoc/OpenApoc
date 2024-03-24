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
#include <iomanip>
#include <limits>
#include <sstream>

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
    {"OpenApoc.Mod", "MaxTileRepair"},
    {"OpenApoc.Mod", "SceneryRepairCostFactor"},
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
    {"OpenApoc.NewFeature", "LoadSameAmmo"},
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

// By default, cityscape and battlescape options list treat all options as boolean values
// But we have some exceptions with different value types that needs to be properly checked
const auto intNotificationsList = {"OpenApoc.Mod.MaxTileRepair"};
const auto floatNotificationsList = {"OpenApoc.Mod.SceneryRepairCostFactor"};

} // namespace
MoreOptions::MoreOptions(sp<GameState> state)
    : Stage(), menuform(ui().getForm("moreoptions")), state(state)
{
}
MoreOptions::~MoreOptions() {}

UString MoreOptions::getOptionFullName(const UString &optionSection,
                                       const UString &optionName) const
{
	const UString fullName = optionSection + "." + optionName;
	return fullName;
}

bool MoreOptions::GetIfOptionInt(const UString &optionFullName) const
{
	const auto isOptionInt = std::find(intNotificationsList.begin(), intNotificationsList.end(),
	                                   optionFullName) != intNotificationsList.end();

	return isOptionInt;
}

bool MoreOptions::GetIfOptionInt(const UString &optionSection, const UString &optionName) const
{
	const auto optionFullName = getOptionFullName(optionSection, optionName);
	const auto isOptionInt = GetIfOptionInt(optionFullName);

	return isOptionInt;
}

bool MoreOptions::GetIfOptionFloat(const UString &optionFullName) const
{
	const auto isOptionFloat =
	    std::find(floatNotificationsList.begin(), floatNotificationsList.end(), optionFullName) !=
	    floatNotificationsList.end();

	return isOptionFloat;
}

bool MoreOptions::GetIfOptionFloat(const UString &optionSection, const UString &optionName) const
{
	const auto optionFullName = getOptionFullName(optionSection, optionName);
	const auto isOptionFloat = GetIfOptionFloat(optionFullName);

	return isOptionFloat;
}

void MoreOptions::saveLists()
{
	const std::list<UString> notificationsList = {"CITY_NOTIFICATIONS_LIST",
	                                              "BATTLE_NOTIFICATIONS_LIST"};

	for (const auto &notification : notificationsList)
	{
		const auto listbox = menuform->findControlTyped<ListBox>(notification);
		for (const auto &control : listbox->Controls)
		{
			const auto name = control->getData<UString>();

			const auto isOptionInt = GetIfOptionInt(*name);

			if (isOptionInt)
			{
				const auto value = std::stoi(std::dynamic_pointer_cast<Label>(control)->getText());
				config().set(*name, value);
				continue;
			}

			const auto isOptionFloat = GetIfOptionFloat(*name);

			if (isOptionFloat)
			{
				const auto value = std::stof(std::dynamic_pointer_cast<Label>(control)->getText());
				config().set(*name, value);
				continue;
			}

			config().set(*name, std::dynamic_pointer_cast<CheckBox>(control)->isChecked());
		}
	}
}

void MoreOptions::loadLists()
{
	saveLists();

	const auto font = ui().getFont("smalfont");

	// Unifying options treatment at both city notification and battle notification
	const auto optionTupleList = {std::make_tuple("CITYLIST_NAME", "Cityscape Options",
	                                              "CITY_NOTIFICATIONS_LIST", &cityscapeList),
	                              std::make_tuple("BATTLELIST_NAME", "Battlescape Options",
	                                              "BATTLE_NOTIFICATIONS_LIST", &battlescapeList)};

	std::list<std::pair<std::list<std::pair<UString, UString>>, sp<ListBox>>>
	    notificationControlPairList = {};

	for (const auto &optionTuple : optionTupleList)
	{
		const auto listNameString = std::get<0>(optionTuple);
		const auto scapeOptionsString = std::get<1>(optionTuple);
		const auto notificationsListString = std::get<2>(optionTuple);
		const auto scapeList = std::get<3>(optionTuple);

		menuform->findControlTyped<Label>(listNameString)->setText(scapeOptionsString);
		const auto *notificationList = scapeList;

		const auto listControl = menuform->findControlTyped<ListBox>(notificationsListString);
		listControl->clear();

		notificationControlPairList.push_back({*notificationList, listControl});
	}

	for (const auto &notificationControlPair : notificationControlPairList)
	{
		const auto &notificationList = notificationControlPair.first;
		const auto &listControl = notificationControlPair.second;

		for (const auto &notification : notificationList)
		{
			const auto fullName = getOptionFullName(notification.first, notification.second);

			const auto isOptionInt = GetIfOptionInt(fullName);

			if (isOptionInt)
			{
				const auto label = mksp<Label>(
				    tr(config().describe(notification.first, notification.second)), font);
				const auto labelText = std::to_string(config().getInt(fullName));
				label->setText(labelText);

				configureOptionControlAndAddToControlListBox(
				    label, notification.first, notification.second, font, listControl);

				continue;
			}

			const auto isOptionFloat = GetIfOptionFloat(fullName);

			if (isOptionFloat)
			{
				auto label = mksp<Label>(
				    tr(config().describe(notification.first, notification.second)), font);

				std::stringstream stream;
				stream << std::fixed << std::setprecision(2) << config().getFloat(fullName);
				const auto labelText = stream.str();

				label->setText(labelText);

				configureOptionControlAndAddToControlListBox(
				    label, notification.first, notification.second, font, listControl);

				continue;
			}

			const auto checkBox = mksp<CheckBox>(fw().data->loadImage("BUTTON_CHECKBOX_TRUE"),
			                                     fw().data->loadImage("BUTTON_CHECKBOX_FALSE"));
			checkBox->setChecked(config().getBool(fullName));

			configureOptionControlAndAddToControlListBox(checkBox, notification.first,
			                                             notification.second, font, listControl);
		}
	}
}

void MoreOptions::configureOptionControlAndAddToControlListBox(const sp<Control> &control,
                                                               const UString &optionSection,
                                                               const UString &optionName,
                                                               const sp<BitmapFont> &font,
                                                               const sp<ListBox> &listControl)
{
	const auto optionFullName = getOptionFullName(optionSection, optionName);

	control->Size = {240, listControl->ItemSize};
	control->setData(mksp<UString>(optionFullName));
	addChildLabelToControl(control, optionSection, optionName, font, listControl);
	listControl->addItem(control);
}

void MoreOptions::addChildLabelToControl(const sp<Control> &control, const UString &optionSection,
                                         const UString &optionName, const sp<BitmapFont> &font,
                                         const sp<ListBox> &listControl)
{
	const auto chidlLabel =
	    control->createChild<Label>(tr(config().describe(optionSection, optionName)), font);
	chidlLabel->Size = {216, listControl->ItemSize};
	chidlLabel->Location = {24, 0};
	chidlLabel->ToolTipText = tr(config().describe(optionSection, optionName));
	chidlLabel->ToolTipFont = font;
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
