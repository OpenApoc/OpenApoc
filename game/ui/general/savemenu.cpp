#include "game/ui/general/savemenu.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/textedit.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "game/state/gamestate.h"
#include "game/ui/general/loadingscreen.h"
#include "game/ui/general/messagebox.h"
#include "game/ui/tileview/battleview.h"
#include "game/ui/tileview/cityview.h"
#include "library/sp.h"
#include <iomanip>
#include <sstream>

#ifdef _MSC_VER
// msvs reports level 3 warning 4996 - std::localtime is unsafe
#pragma warning(disable : 4996)
#endif

namespace OpenApoc
{
const UString existingSaveItemId = "EXISTING_SAVE_SLOT";
const UString newSaveItemId = "NEW_SAVE_SLOT";

SaveMenu::SaveMenu(SaveMenuAction saveMenuAction, sp<GameState> state)
    : Stage(), menuform(ui().getForm("savemenu")), currentState(state),
      currentAction(saveMenuAction)
{
	activeTextEdit = nullptr;
}

SaveMenu::~SaveMenu() = default;

void SaveMenu::begin()
{
	if (currentState)
	{
		menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(currentState->getPlayerBalance());
	}

	auto saveListBox = menuform->findControlTyped<ListBox>("LISTBOX_OPTIONS");

	UString titleLabelName;
	bool newItemSlotVisible = false;
	bool showAutoSaves = false;
	switch (currentAction)
	{
		case SaveMenuAction::LoadNewGame:
		case SaveMenuAction::Load:
			titleLabelName = "LABEL_LOADGAME";
			showAutoSaves = true;
			break;
		case SaveMenuAction::Save:
			titleLabelName = "LABEL_SAVEGAME";
			newItemSlotVisible = true;
			break;
		case SaveMenuAction::Delete:
			titleLabelName = "LABEL_DELETEGAME";
			break;
	}

	if (!titleLabelName.empty())
	{
		auto label = menuform->findControlTyped<Label>(titleLabelName);
		if (label)
		{
			label->setVisible(true);
		}
	}

	if (newItemSlotVisible == true)
	{
		auto newItemSlot = saveListBox->findControl(newSaveItemId);
		if (newItemSlot != nullptr)
		{
			newItemSlot->setVisible(true);
		}
	}

	// load menu items
	auto saves = saveManager.getSaveList();
	auto existingSlotControl = saveListBox->findControl(existingSaveItemId);
	if (existingSlotControl != nullptr)
	{
		for (auto it = saves.begin(); it != saves.end(); ++it)
		{
			if (it->getType() != SaveType::Manual && !showAutoSaves)
			{ // skip quicksave and autosave on save screen
				continue;
			}

			auto newControl = existingSlotControl->copyTo(saveListBox);
			newControl->setData(mksp<SaveMetadata>(*it));
			auto nameLabel = newControl->findControlTyped<Label>("LABEL_NAME");
			if (nameLabel != nullptr)
			{
				nameLabel->setText(it->getName());
			}

			auto saveTimeLabel = newControl->findControlTyped<Label>("LABEL_TIME");
			if (saveTimeLabel != nullptr)
			{
				std::time_t timestamp = it->getCreationDate();
				struct tm *tminfo = std::localtime(&timestamp);
				std::stringstream ss;
				if (timestamp != 0 && tminfo != nullptr)
				{
					char temp_time[1024];
					strftime(temp_time, sizeof(temp_time), "%d/%m/%y %T", tminfo);
					ss << temp_time;
				}
				saveTimeLabel->setText(ss.str());
			}

			GameTime gameTime(it->getGameTicks());
			auto gameDayLabel = newControl->findControlTyped<Label>("LABEL_INGAME_DAY");
			if (gameDayLabel != nullptr)
			{
				if (it->getGameTicks() == 0 && gameTime.getDay() == 0)
				{
					gameDayLabel->setText("");
				}
				else
				{
					gameDayLabel->setText("Day " + std::to_string(gameTime.getDay()));
				}
			}
			auto gameTimeLabel = newControl->findControlTyped<Label>("LABEL_INGAME_TIME");
			if (gameTimeLabel != nullptr)
			{
				if (it->getGameTicks() == 0 && gameTime.getDay() == 0)
				{
					gameTimeLabel->setText("");
				}
				else
				{
					gameTimeLabel->setText(gameTime.getLongTimeString());
				}
			}

			auto difficultyLabel = newControl->findControlTyped<Label>("LABEL_DIFFICULTY");
			if (difficultyLabel != nullptr)
			{
				difficultyLabel->setText(it->getDifficulty());
			}

			auto saveNameTextEdit = newControl->findControlTyped<TextEdit>("TEXTEDIT_SAVE_NAME");
			if (saveNameTextEdit != nullptr)
			{
				saveNameTextEdit->setVisible(false);
			}
		}
		existingSlotControl->setVisible(false);
	}
}

void SaveMenu::pause() {}

void SaveMenu::resume() {}

void SaveMenu::finish() {}

void SaveMenu::clearTextEdit(sp<TextEdit> textEdit)
{
	auto e = OpenApoc::FormsEvent();
	e.forms().RaisedBy = textEdit->shared_from_this();
	e.forms().EventFlag = FormEventType::LostFocus;
	textEdit->eventOccured(&e);

	// reset text
	textEdit->setVisible(false);
	auto listItem = textEdit->getParent();
	if (listItem)
	{
		auto nameLabel = listItem->findControlTyped<Label>("LABEL_NAME");
		if (nameLabel)
		{
			nameLabel->setVisible(true);
		}
	}
	activeTextEdit = nullptr;
}

void SaveMenu::beginEditing(sp<TextEdit> textEdit, sp<TextEdit> activeTextEdit)
{
	if (activeTextEdit != nullptr)
	{
		clearTextEdit(activeTextEdit);
	}

	auto e = OpenApoc::FormsEvent();
	e.forms().RaisedBy = textEdit->shared_from_this();
	e.forms().EventFlag = FormEventType::MouseClick;
	textEdit->eventOccured(&e);
	textEdit->setVisible(true);
	textEdit->setAllowedCharacters(
	    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,. -_");
	textEdit->setTextMaxSize(32);
	auto listItem = textEdit->getParent();
	if (listItem)
	{
		auto nameLabel = listItem->findControlTyped<Label>("LABEL_NAME");
		if (nameLabel)
		{
			nameLabel->setVisible(false);
			if (nameLabel->getParent() && nameLabel->getParent()->Name != "NEW_SAVE_SLOT")
			{
				textEdit->setText(nameLabel->getText());
			}
			else
			{
				textEdit->setText("");
			}
		}
	}
}

void SaveMenu::loadWithWarning(sp<Control> parent)
{
	if (parent->Name == existingSaveItemId)
	{
		sp<SaveMetadata> slot = parent->getData<SaveMetadata>();
		if (slot != nullptr)
		{
			std::function<void()> onSuccess = std::function<void()>([this, slot] {
				auto state = mksp<GameState>();
				auto task = saveManager.loadGame(*slot, state);
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<LoadingScreen>(nullptr, std::move(task), [state]() -> sp<Stage> {
					     if (state->current_battle)
					     {
						     return mksp<BattleView>(state);
					     }
					     else
					     {
						     return mksp<CityView>(state);
					     }
				     })});
			});
			sp<MessageBox> messageBox = mksp<MessageBox>(
			    MessageBox("Load game", "Unsaved progress will be lost. Continue?",
			               MessageBox::ButtonOptions::YesNo, std::move(onSuccess), nullptr));

			fw().stageQueueCommand({StageCmd::Command::PUSH, messageBox});
		}
	}
}

void SaveMenu::tryToLoadGame(sp<Control> slotControl)
{
	if (slotControl->Name == existingSaveItemId)
	{
		sp<SaveMetadata> slot = slotControl->getData<SaveMetadata>();
		if (slot != nullptr)
		{
			auto state = mksp<GameState>();
			auto task = saveManager.loadGame(*slot, state);
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH,
			     mksp<LoadingScreen>(nullptr, std::move(task), [state]() -> sp<Stage> {
				     if (state->current_battle)
				     {
					     return mksp<BattleView>(state);
				     }
				     else
				     {
					     return mksp<CityView>(state);
				     }
			     })});
		}
	}
}

void SaveMenu::tryToSaveGame(const UString &saveName, sp<Control> parent)
{
	if (parent->Name == newSaveItemId)
	{
		if (saveManager.newSaveGame(saveName, currentState))
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
		}
		else
		{
			clearTextEdit(activeTextEdit);
		}
	}
	else
	{
		sp<SaveMetadata> slot = parent->getData<SaveMetadata>();
		std::function<void()> onSuccess = std::function<void()>([this, slot, saveName] {
			if (saveManager.overrideGame(*slot, saveName, currentState))
			{
				fw().stageQueueCommand({StageCmd::Command::POP});
			}
			else
			{
				clearTextEdit(activeTextEdit);
			}
		});
		std::function<void()> onCancel =
		    std::function<void()>([this] { clearTextEdit(activeTextEdit); });
		sp<MessageBox> messageBox = mksp<MessageBox>(MessageBox(
		    "Override saved game", "Do you really want to override " + slot->getName() + "?",
		    MessageBox::ButtonOptions::YesNo, std::move(onSuccess), std::move(onCancel)));

		fw().stageQueueCommand({StageCmd::Command::PUSH, messageBox});
	}
}

void SaveMenu::tryToDeleteSavedGame(sp<Control> &slotControl)
{
	sp<SaveMetadata> slot = slotControl->getData<SaveMetadata>();
	std::function<void()> onSuccess = std::function<void()>([this, slotControl, slot] {
		if (saveManager.deleteGame(slot))
		{
			// no way to pop
			slotControl->setVisible(false);
		}
	});
	sp<MessageBox> messageBox = mksp<MessageBox>(
	    MessageBox("Delete saved game", "Do you really want to delete " + slot->getName() + "?",
	               MessageBox::ButtonOptions::YesNo, std::move(onSuccess), nullptr));

	fw().stageQueueCommand({StageCmd::Command::PUSH, messageBox});
}

void SaveMenu::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (activeTextEdit && activeTextEdit->isFocused())
			return;
		if (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_RETURN ||
		    e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_QUIT")->click();
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION)
	{
		switch (e->forms().EventFlag)
		{
			case FormEventType::ButtonClick:
				if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
				{
					fw().stageQueueCommand({StageCmd::Command::POP});
				}
				else
				{
					sp<Control> slotControl;
					switch (currentAction)
					{
						case SaveMenuAction::LoadNewGame:
							slotControl = e->forms().RaisedBy->getParent();
							if (slotControl)
							{
								tryToLoadGame(slotControl);
							}
							break;
						case SaveMenuAction::Load:
							slotControl = e->forms().RaisedBy->getParent();
							if (slotControl)
							{
								loadWithWarning(slotControl);
							}
							break;
						case SaveMenuAction::Save:
							slotControl = e->forms().RaisedBy->getParent();
							if (slotControl)
							{
								auto nameEdit =
								    slotControl->findControlTyped<TextEdit>("TEXTEDIT_SAVE_NAME");
								if (nameEdit && nameEdit != activeTextEdit)
								{
									beginEditing(nameEdit, activeTextEdit);
									activeTextEdit = nameEdit;
								}
							}
							break;
						case SaveMenuAction::Delete:
							slotControl = e->forms().RaisedBy->getParent();
							if (slotControl)
							{
								tryToDeleteSavedGame(slotControl);
							}
							break;
					}
				}
				break;
			case FormEventType::TextEditFinish:
			{
				sp<TextEdit> textEdit = std::static_pointer_cast<TextEdit>(e->forms().RaisedBy);
				auto slotControl = e->forms().RaisedBy->getParent();
				if (!slotControl || !textEdit || (textEdit != activeTextEdit))
				{
					return;
				}

				tryToSaveGame(textEdit->getText(), slotControl);
				break;
			}
			default:
				break;
		}
	}
}

void SaveMenu::update() { menuform->update(); }

void SaveMenu::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	fw().renderer->drawFilledRect(Vec2<float>(0, 0),
	                              Vec2<float>(fw().displayGetWidth(), fw().displayGetHeight()),
	                              Colour(0, 0, 0, 128));
	menuform->render();
}

bool SaveMenu::isTransition() { return false; }
}; // namespace OpenApoc
