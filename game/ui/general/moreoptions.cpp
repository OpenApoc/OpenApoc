#include "game/ui/general/moreoptions.h"
#include "forms/checkbox.h"
#include "forms/form.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/scrollbar.h"
#include "forms/textbutton.h"
#include "forms/textedit.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"
#include "game/state/shared/organisation.h"
#include <forms/graphic.h>
#include <functional>
#include <iomanip>
#include <limits>
#include <regex>
#include <sstream>

namespace OpenApoc
{
namespace
{
static const std::list<std::pair<UString, UString>> cityscapeList = {
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
    {"OpenApoc.NewFeature", "ShowCurrentDimensionVehicles"},
    {"OpenApoc.NewFeature", "ShowNonXCOMVehiclesPrefix"},
    {"OpenApoc.Mod", "MaxTileRepair"},
    {"OpenApoc.Mod", "SceneryRepairCostFactor"},
    {"OpenApoc.Mod", "RaidHostileAction"},
    {"OpenApoc.Mod", "CrashingVehicles"},
    {"OpenApoc.Mod", "InvulnerableRoads"},
    {"OpenApoc.Mod", "ATVTank"},
    {"OpenApoc.Mod", "ATVAPC"},
};

static const std::list<std::pair<UString, UString>> battlescapeList = {
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
static const std::list<std::pair<UString, UString>> vanillaList = {
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
static const auto INT_NOTIFICATIONS_LIST = {"OpenApoc.Mod.MaxTileRepair"};
static const auto FLOAT_NOTIFICATIONS_LIST = {"OpenApoc.Mod.SceneryRepairCostFactor"};

static const std::regex NUMERIC_CHARS_REGEX("[^0-9.]");
sp<BitmapFont> font = nullptr;

static const float NUMERIC_OPTION_MAX_LIMIT = 100.0;
static const float NUMERIC_OPTION_MIN_LIMIT = 0;

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

bool MoreOptions::getIfOptionInt(const UString &optionFullName) const
{
	const auto isOptionInt = std::find(INT_NOTIFICATIONS_LIST.begin(), INT_NOTIFICATIONS_LIST.end(),
	                                   optionFullName) != INT_NOTIFICATIONS_LIST.end();

	return isOptionInt;
}

bool MoreOptions::getIfOptionInt(const UString &optionSection, const UString &optionName) const
{
	const auto optionFullName = getOptionFullName(optionSection, optionName);
	const auto isOptionInt = getIfOptionInt(optionFullName);

	return isOptionInt;
}

bool MoreOptions::getIfOptionFloat(const UString &optionFullName) const
{
	const auto isOptionFloat =
	    std::find(FLOAT_NOTIFICATIONS_LIST.begin(), FLOAT_NOTIFICATIONS_LIST.end(),
	              optionFullName) != FLOAT_NOTIFICATIONS_LIST.end();

	return isOptionFloat;
}

bool MoreOptions::getIfOptionFloat(const UString &optionSection, const UString &optionName) const
{
	const auto optionFullName = getOptionFullName(optionSection, optionName);
	const auto isOptionFloat = getIfOptionFloat(optionFullName);

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

			const auto isOptionInt = getIfOptionInt(*name);

			if (isOptionInt)
			{
				// Using default value if getInt fails
				auto value = config().getInt(*name);

				try
				{
					value = std::stoi(std::dynamic_pointer_cast<TextEdit>(control)->getText());

					if (value > NUMERIC_OPTION_MAX_LIMIT)
						value = (int)NUMERIC_OPTION_MAX_LIMIT;
					else if (value < NUMERIC_OPTION_MIN_LIMIT)
						value = (int)NUMERIC_OPTION_MIN_LIMIT;
				}
				catch (const std::exception &)
				{
				}

				config().set(*name, value);
				continue;
			}

			const auto isOptionFloat = getIfOptionFloat(*name);

			if (isOptionFloat)
			{
				// Using default value if getFloat fails
				auto value = config().getFloat(*name);

				try
				{
					value = std::stof(std::dynamic_pointer_cast<TextEdit>(control)->getText());

					if (value > NUMERIC_OPTION_MAX_LIMIT)
						value = NUMERIC_OPTION_MAX_LIMIT;
					else if (value < NUMERIC_OPTION_MIN_LIMIT)
						value = NUMERIC_OPTION_MIN_LIMIT;
				}
				catch (const std::exception &)
				{
				}

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

	font = ui().getFont("smalfont");

	// This text edit list will be used to remove focus from a text edit when another one is clicked
	std::list<sp<TextEdit>> textEditList = {};

	for (const auto &notificationControlPair : notificationControlPairList)
	{
		const auto &notificationList = notificationControlPair.first;
		const auto &listControl = notificationControlPair.second;

		for (const auto &notification : notificationList)
		{
			const auto fullName = getOptionFullName(notification.first, notification.second);

			const auto isOptionInt = getIfOptionInt(fullName);

			if (isOptionInt)
			{
				const auto configValue = config().getInt(fullName);
				const auto labelText = std::to_string(configValue);

				const auto textEdit = createTextEditForNumericOptions(
				    notification.first, notification.second, listControl, labelText);

				const auto buttonUpCallback = [this, textEdit, fullName](const Event *)
				{
					try
					{
						auto value =
						    std::stoi(std::dynamic_pointer_cast<TextEdit>(textEdit)->getText());

						if (value >= NUMERIC_OPTION_MAX_LIMIT)
							return;

						value += 1;

						const auto labelText = std::to_string(value);
						textEdit->setText(labelText);
					}
					catch (const std::exception &)
					{
					}
				};

				const auto buttonDownCallback = [this, textEdit, fullName](const Event *)
				{
					try
					{
						auto value =
						    std::stoi(std::dynamic_pointer_cast<TextEdit>(textEdit)->getText());

						if (value <= NUMERIC_OPTION_MIN_LIMIT)
							return;

						value -= 1;

						const auto labelText = std::to_string(value);
						textEdit->setText(labelText);
					}
					catch (const std::exception &)
					{
					}
				};

				addButtonsToNumericOption(textEdit, listControl, buttonUpCallback,
				                          buttonDownCallback);

				addChildLabelToControl(textEdit, notification.first, notification.second,
				                       listControl, 65);

				listControl->addItem(textEdit);
				textEditList.push_back(textEdit);

				continue;
			}

			const auto isOptionFloat = getIfOptionFloat(fullName);

			if (isOptionFloat)
			{
				const auto configValue = config().getFloat(fullName);

				std::stringstream stream;
				stream << std::fixed << std::setprecision(1) << configValue;
				const auto labelText = stream.str();

				const auto textEdit = createTextEditForNumericOptions(
				    notification.first, notification.second, listControl, labelText);

				const auto buttonUpCallback = [this, textEdit, fullName](const Event *)
				{
					try
					{
						auto value =
						    std::stof(std::dynamic_pointer_cast<TextEdit>(textEdit)->getText());

						if (value >= NUMERIC_OPTION_MAX_LIMIT)
							return;

						value += (float)0.1;

						std::stringstream stream;
						stream << std::fixed << std::setprecision(1) << value;
						const auto labelText = stream.str();

						textEdit->setText(labelText);
					}
					catch (const std::exception &)
					{
					}
				};

				const auto buttonDownCallback = [this, textEdit, fullName](const Event *)
				{
					try
					{
						auto value =
						    std::stof(std::dynamic_pointer_cast<TextEdit>(textEdit)->getText());

						if (value <= NUMERIC_OPTION_MIN_LIMIT)
							return;

						value -= (float)0.1;

						std::stringstream stream;
						stream << std::fixed << std::setprecision(1) << value;
						const auto labelText = stream.str();

						textEdit->setText(labelText);
					}
					catch (const std::exception &)
					{
					}
				};

				addButtonsToNumericOption(textEdit, listControl, buttonUpCallback,
				                          buttonDownCallback);

				addChildLabelToControl(textEdit, notification.first, notification.second,
				                       listControl, 65);

				listControl->addItem(textEdit);
				textEditList.push_back(textEdit);

				continue;
			}

			const auto checkBox = mksp<CheckBox>(fw().data->loadImage("BUTTON_CHECKBOX_TRUE"),
			                                     fw().data->loadImage("BUTTON_CHECKBOX_FALSE"));
			checkBox->setChecked(config().getBool(fullName));

			configureOptionControlAndAddToControlListBox(checkBox, notification.first,
			                                             notification.second, listControl, 24);
		}
	}

	addFocusControlCallbackToNumberTextEdit(textEditList);
}

void MoreOptions::configureOptionControlAndAddToControlListBox(const sp<Control> &control,
                                                               const UString &optionSection,
                                                               const UString &optionName,
                                                               const sp<ListBox> &listControl,
                                                               const int &labelLocationHeight)
{
	const auto optionFullName = getOptionFullName(optionSection, optionName);

	control->Size = {240, listControl->ItemSize};
	control->setData(mksp<UString>(optionFullName));
	addChildLabelToControl(control, optionSection, optionName, listControl, labelLocationHeight);
	listControl->addItem(control);
}

sp<TextEdit> MoreOptions::createTextEditForNumericOptions(const UString &optionSection,
                                                          const UString &optionName,
                                                          const sp<ListBox> &listControl,
                                                          const UString &labelText) const
{
	const auto optionFullName = getOptionFullName(optionSection, optionName);

	// Create textEdit
	const auto textEdit = mksp<TextEdit>(tr(config().describe(optionSection, optionName)), font);

	textEdit->setText(labelText);

	textEdit->Size = {25, listControl->ItemSize};
	textEdit->setData(mksp<UString>(optionFullName));

	// Add common callbacks

	// Validating if text changed and removing any non-numeric value if it was added
	textEdit->addCallback(FormEventType::TextChanged,
	                      [textEdit](Event *e)
	                      {
		                      try
		                      {
			                      const auto textValue = textEdit->getText();

			                      if (textValue.length() == 0)
			                      {
				                      return;
			                      }

			                      const char *lastChar = &textValue[textValue.length() - 1];

			                      std::cmatch results;

			                      if (std::regex_search(lastChar, results, NUMERIC_CHARS_REGEX))
			                      {
				                      textEdit->setText(
				                          textValue.substr(0, textValue.length() - 1));
				                      return;
			                      }
		                      }
		                      catch (const std::exception &)
		                      {
			                      textEdit->setText(std::to_string(0));
		                      }
	                      });

	// Callbacks to validate if textEdit content is a numeric value, update with config values
	// otherwise
	textEdit->addCallback(FormEventType::TextEditCancel,
	                      [textEdit](Event *e)
	                      {
		                      try
		                      {
			                      const auto textValue = textEdit->getText();
			                      std::stof(textValue);
		                      }
		                      catch (const std::exception &)
		                      {
			                      textEdit->setText(std::to_string(0));
		                      }
	                      });

	textEdit->addCallback(FormEventType::TextEditFinish,
	                      [textEdit](Event *e)
	                      {
		                      try
		                      {
			                      const auto textValue = textEdit->getText();
			                      std::stof(textValue);
		                      }
		                      catch (const std::exception &)
		                      {
			                      textEdit->setText(std::to_string(0));
		                      }
	                      });

	return textEdit;
}

void MoreOptions::addButtonsToNumericOption(
    const sp<Control> &control, const sp<ListBox> &listControl,
    const std::function<void(FormsEvent *e)> &buttonUpClickCallback,
    const std::function<void(FormsEvent *e)> &buttonDownClickCallback)
{
	const auto buttonUp = control->createChild<GraphicButton>(
	    fw().data->loadImage(
	        "PCK:xcom3/ufodata/icons.pck:xcom3/ufodata/icons.tab:13:ui/menuopt.pal"),
	    fw().data->loadImage(
	        "PCK:xcom3/ufodata/icons.pck:xcom3/ufodata/icons.tab:14:ui/menuopt.pal"));

	buttonUp->Size = {20, listControl->ItemSize};
	buttonUp->Location = {27, 0};

	buttonUp->addCallback(FormEventType::ButtonClick, buttonUpClickCallback);

	const auto buttonDown = control->createChild<GraphicButton>(
	    fw().data->loadImage(
	        "PCK:xcom3/ufodata/icons.pck:xcom3/ufodata/icons.tab:15:ui/menuopt.pal"),
	    fw().data->loadImage(
	        "PCK:xcom3/ufodata/icons.pck:xcom3/ufodata/icons.tab:16:ui/menuopt.pal"));

	buttonDown->Size = {20, listControl->ItemSize};
	buttonDown->Location = {45, 0};

	buttonDown->addCallback(FormEventType::ButtonClick, buttonDownClickCallback);
}

void MoreOptions::addChildLabelToControl(const sp<Control> &control, const UString &optionSection,
                                         const UString &optionName, const sp<ListBox> &listControl,
                                         const int &labelLocationHeight)
{
	const auto chidlLabel =
	    control->createChild<Label>(tr(config().describe(optionSection, optionName)), font);
	chidlLabel->Size = {216, listControl->ItemSize};
	chidlLabel->Location = {labelLocationHeight, 0};
	chidlLabel->ToolTipText = tr(config().describe(optionSection, optionName));
	chidlLabel->ToolTipFont = font;
}

void MoreOptions::addFocusControlCallbackToNumberTextEdit(
    const std::list<sp<TextEdit>> &textEditList)
{
	for (const auto &textEdit : textEditList)
	{
		textEdit->addCallback(FormEventType::MouseClick,
		                      [textEditList, textEdit](Event *e)
		                      {
			                      for (auto &textEditItem : textEditList)
			                      {
				                      // When the user clicks on a text edit, every other text edit
				                      // with focus will lose it
				                      // That way, only the last clicked text edit will have focus
				                      if (textEditItem != textEdit && textEditItem->isFocused())
				                      {
					                      // TODO: find a way to remove focus
				                      }
			                      }
		                      });
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
