#include "game/ui/general/savemenu.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/ui/general/loadingscreen.h"
#include "game/ui/general/messagebox.h"
#include "library/sp.h"
#include <iomanip>

// msvs reports level 3 warning 4996 - std::localtime is unsafe
#pragma warning(disable : 4996)

namespace OpenApoc
{
const UString existingSaveItemId = "EXISTING_SAVE_SLOT";
const UString newSaveItemId = "NEW_SAVE_SLOT";

SaveMenu::SaveMenu(SaveMenuAction saveMenuAction, sp<GameState> state)
    : Stage(), menuform(ui().GetForm("FORM_SAVEMENU")), currentState(state),
      currentAction(saveMenuAction)
{
	activeTextEdit = nullptr;
}

SaveMenu::~SaveMenu() {}

void SaveMenu::Begin()
{
	menuform->FindControlTyped<Label>("TEXT_FUNDS")->SetText(currentState->getPlayerBalance());

	auto saveListBox = menuform->FindControlTyped<ListBox>("LISTBOX_OPTIONS");

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
		auto label = menuform->FindControlTyped<Label>(titleLabelName);
		if (label)
		{
			label->Visible = true;
		}
	}

	if (newItemSlotVisible == true)
	{
		auto newItemSlot = saveListBox->FindControl(newSaveItemId);
		if (newItemSlot != nullptr)
		{
			newItemSlot->Visible = true;
		}
	}

	// load menu items
	auto saves = saveManager.getSaveList();
	auto existingSlotControl = saveListBox->FindControl(existingSaveItemId);
	if (existingSlotControl != nullptr)
	{
		for (auto it = saves.begin(); it != saves.end(); ++it)
		{
			if (it->getType() != SaveType::Manual && !showAutoSaves)
			{ // skip quicksave and autosave on save screen
				continue;
			}

			auto newControl = existingSlotControl->CopyTo(saveListBox);
			newControl->SetData(mksp<SaveMetadata>(*it));
			auto nameLabel = newControl->FindControlTyped<Label>("LABEL_NAME");
			if (nameLabel != nullptr)
			{
				nameLabel->SetText(it->getName());
			}

			auto saveTimeLabel = newControl->FindControlTyped<Label>("LABEL_TIME");
			if (saveTimeLabel != nullptr)
			{
				std::time_t timestamp = it->getCreationDate();
				struct tm *tminfo = std::localtime(&timestamp);
				std::stringstream ss;
				if (timestamp != 0 && tminfo != nullptr)
				{
					ss << std::put_time(tminfo, "%d/%m/%y %T");
				}
				saveTimeLabel->SetText(ss.str());
			}

			GameTime gameTime(it->getGameTicks());
			auto gameDayLabel = newControl->FindControlTyped<Label>("LABEL_INGAME_DAY");
			if (gameDayLabel != nullptr)
			{
				if (it->getGameTicks() == 0 && gameTime.getDay() == 0)
				{
					gameDayLabel->SetText("");
				}
				else
				{
					gameDayLabel->SetText("Day " + std::to_string(gameTime.getDay()));
				}
			}
			auto gameTimeLabel = newControl->FindControlTyped<Label>("LABEL_INGAME_TIME");
			if (gameTimeLabel != nullptr)
			{
				if (it->getGameTicks() == 0 && gameTime.getDay() == 0)
				{
					gameTimeLabel->SetText("");
				}
				else
				{
					gameTimeLabel->SetText(gameTime.getTimeString());
				}
			}

			auto difficultyLabel = newControl->FindControlTyped<Label>("LABEL_DIFFICULTY");
			if (difficultyLabel != nullptr)
			{
				difficultyLabel->SetText(it->getDifficulty());
			}

			auto saveNameTextEdit = newControl->FindControlTyped<TextEdit>("TEXTEDIT_SAVE_NAME");
			if (currentAction != SaveMenuAction::Save && saveNameTextEdit != nullptr)
			{
				saveNameTextEdit->Visible = false;
			}
		}
		existingSlotControl->Visible = false;
	}
}

void SaveMenu::Pause() {}

void SaveMenu::Resume() {}

void SaveMenu::Finish() {}

void SaveMenu::clearTextEdit(sp<TextEdit> textEdit)
{
	auto e = OpenApoc::FormsEvent();
	e.Forms().RaisedBy = textEdit->shared_from_this();
	e.Forms().EventFlag = FormEventType::LostFocus;
	textEdit->EventOccured(&e);

	// reset text
	textEdit->Visible = false;
	auto listItem = textEdit->GetParent();
	if (listItem)
	{
		auto nameLabel = listItem->FindControlTyped<Label>("LABEL_NAME");
		if (nameLabel)
		{
			nameLabel->Visible = true;
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
	e.Forms().RaisedBy = textEdit->shared_from_this();
	e.Forms().EventFlag = FormEventType::MouseClick;
	textEdit->EventOccured(&e);
	textEdit->Visible = true;
	textEdit->SetAllowedCharacters(
	    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,. -_");
	textEdit->SetTextMaxSize(32);
	auto listItem = textEdit->GetParent();
	if (listItem)
	{
		auto nameLabel = listItem->FindControlTyped<Label>("LABEL_NAME");
		if (nameLabel)
		{
			nameLabel->Visible = false;
			if (nameLabel->GetParent() && nameLabel->GetParent()->Name != "NEW_SAVE_SLOT")
			{
				textEdit->SetText(nameLabel->GetText());
			}
			else
			{
				textEdit->SetText("");
			}
		}
	}
}

void SaveMenu::loadWithWarning(sp<Control> parent)
{
	if (parent->Name == existingSaveItemId)
	{
		sp<SaveMetadata> slot = parent->GetData<SaveMetadata>();
		if (slot != nullptr)
		{
			std::function<void()> onSuccess = std::function<void()>([this, slot] {
				stageCmd.nextStage = mksp<LoadingScreen>(std::move(saveManager.loadGame(*slot)));
				stageCmd.cmd = StageCmd::Command::PUSH;
			});
			sp<MessageBox> messageBox = mksp<MessageBox>(
			    MessageBox("Load game", "Unsaved progress will be lost. Continue?",
			               MessageBox::ButtonOptions::YesNo, std::move(onSuccess), nullptr));

			stageCmd.nextStage = messageBox;
			stageCmd.cmd = StageCmd::Command::PUSH;
		}
	}
}

void SaveMenu::tryToLoadGame(sp<Control> slotControl)
{
	if (slotControl->Name == existingSaveItemId)
	{
		sp<SaveMetadata> slot = slotControl->GetData<SaveMetadata>();
		if (slot != nullptr)
		{
			stageCmd.nextStage = mksp<LoadingScreen>(std::move(saveManager.loadGame(*slot)));
			stageCmd.cmd = StageCmd::Command::PUSH;
		}
	}
}

void SaveMenu::tryToSaveGame(const UString &saveName, sp<Control> parent)
{
	if (parent->Name == newSaveItemId)
	{
		if (saveManager.newSaveGame(saveName, currentState))
		{
			stageCmd.cmd = StageCmd::Command::POP;
		}
		else
		{
			clearTextEdit(activeTextEdit);
		}
	}
	else
	{
		sp<SaveMetadata> slot = parent->GetData<SaveMetadata>();
		std::function<void()> onSuccess = std::function<void()>([this, slot, saveName] {
			if (saveManager.overrideGame(*slot, saveName, currentState))
			{
				stageCmd.cmd = StageCmd::Command::POP;
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

		stageCmd.nextStage = messageBox;
		stageCmd.cmd = StageCmd::Command::PUSH;
	}
}

void SaveMenu::tryToDeleteSavedGame(sp<Control> &slotControl)
{
	sp<SaveMetadata> slot = slotControl->GetData<SaveMetadata>();
	std::function<void()> onSuccess = std::function<void()>([this, slotControl, slot] {
		if (saveManager.deleteGame(slot))
		{
			// no way to pop
			slotControl->Visible = false;
		}
	});
	sp<MessageBox> messageBox = mksp<MessageBox>(
	    MessageBox("Delete saved game", "Do you really want to delete " + slot->getName() + "?",
	               MessageBox::ButtonOptions::YesNo, std::move(onSuccess), nullptr));

	stageCmd.nextStage = messageBox;
	stageCmd.cmd = StageCmd::Command::PUSH;
}

void SaveMenu::EventOccurred(Event *e)
{
	menuform->EventOccured(e);

	if (e->Type() == EVENT_KEY_DOWN)
	{
		if (e->Keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type() == EVENT_FORM_INTERACTION)
	{
		switch (e->Forms().EventFlag)
		{
			case FormEventType::ButtonClick:
				if (e->Forms().RaisedBy->Name == "BUTTON_QUIT")
				{
					stageCmd.cmd = StageCmd::Command::POP;
				}
				else
				{
					sp<Control> slotControl;
					switch (currentAction)
					{
						case SaveMenuAction::LoadNewGame:
							slotControl = e->Forms().RaisedBy->GetParent();
							if (slotControl)
							{
								tryToLoadGame(slotControl);
							}
							break;
						case SaveMenuAction::Load:
							slotControl = e->Forms().RaisedBy->GetParent();
							if (slotControl)
							{
								loadWithWarning(slotControl);
							}
							break;
						case SaveMenuAction::Save:
							slotControl = e->Forms().RaisedBy->GetParent();
							if (slotControl)
							{
								auto nameEdit =
								    slotControl->FindControlTyped<TextEdit>("TEXTEDIT_SAVE_NAME");
								if (nameEdit && nameEdit != activeTextEdit)
								{
									beginEditing(nameEdit, activeTextEdit);
									activeTextEdit = nameEdit;
								}
							}
							break;
						case SaveMenuAction::Delete:
							slotControl = e->Forms().RaisedBy->GetParent();
							if (slotControl)
							{
								tryToDeleteSavedGame(slotControl);
							}
							break;
					}
				}
				break;
			case FormEventType::TextEditFinish:
				sp<TextEdit> textEdit = std::static_pointer_cast<TextEdit>(e->Forms().RaisedBy);
				auto slotControl = e->Forms().RaisedBy->GetParent();
				if (!slotControl || !textEdit || (textEdit != activeTextEdit))
				{
					return;
				}

				tryToSaveGame(textEdit->GetText(), slotControl);
				break;
		}
	}
}

void SaveMenu::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void SaveMenu::Render()
{
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	fw().renderer->drawFilledRect(Vec2<float>(0, 0),
	                              Vec2<float>(fw().Display_GetWidth(), fw().Display_GetHeight()),
	                              Colour(0, 0, 0, 128));
	menuform->Render();
}

bool SaveMenu::IsTransition() { return false; }
}; // namespace OpenApoc
