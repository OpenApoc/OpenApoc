#include "game/ui/general/savemenu.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/ui/city/cityview.h"
#include "game/ui/general/loadingscreen.h"
#include "library/sp.h"
#include <iomanip>
#include <tuple>

// msvs reports level 3 warning 4996 - std::localtime is unsafe
#pragma warning(disable : 4996)

namespace OpenApoc
{
const UString existingSaveItemId = "EXISTING_SAVE_SLOT";
const UString newSaveItemId = "NEW_SAVE_SLOT";

SaveMenu::SaveMenu(SaveMenuAction saveMenuAction, sp<GameState> state)
    : Stage(), menuform(ui().GetForm("FORM_SAVEMENU")), currentAction(saveMenuAction),
      currentState(state)
{
}
SaveMenu::~SaveMenu() {}

void SaveMenu::Begin()
{
	auto saveListBox = menuform->FindControlTyped<ListBox>("LISTBOX_OPTIONS");
	auto newItemSlot = saveListBox->FindControl(newSaveItemId);

	// hide ui based on action
	sp<Label> labelToHide;
	if (currentAction != SaveMenuAction::Save)
	{
		labelToHide = menuform->FindControlTyped<Label>("LABEL_SAVEGAME");
		if (saveListBox != nullptr)
		{
			if (newItemSlot != nullptr)
			{
				newItemSlot->Visible = false;
			}
		}
	}
	else
	{
		labelToHide = menuform->FindControlTyped<Label>("LABEL_LOADGAME");
	}

	if (labelToHide != nullptr)
	{
		labelToHide->Visible = false;
	}

	// load menu items
	auto saves = saveManager.getSaveList();
	auto existingSlotControl = saveListBox->FindControl(existingSaveItemId);
	if (existingSlotControl != nullptr)
	{
		for (auto it = saves.begin(); it != saves.end(); ++it)
		{
			if (it->getType() != Manual && currentAction == Save)
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
					ss << std::put_time(tminfo, "%c %Z");
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
					auto seconds = it->getGameTicks() / TICKS_PER_SECOND;
					auto minutes = seconds / 60;
					auto hours = minutes / 60;

					unsigned secondsClamped = seconds % 60;
					unsigned minutesClamped = minutes % 60;
					unsigned hoursClamped = hours % 24;

					auto timeString = UString::format("%02u:%02u:%02u", hoursClamped,
					                                  minutesClamped, secondsClamped);
					gameTimeLabel->SetText(timeString);
				}
			}

			auto difficultyLabel = newControl->FindControlTyped<Label>("LABEL_DIFFICULTY");
			if (difficultyLabel != nullptr)
			{
				difficultyLabel->SetText(it->getDifficulty());
			}
		}
		existingSlotControl->Visible = false;
	}
}
void SaveMenu::Pause() {}

void SaveMenu::Resume() {}

void SaveMenu::Finish() {}

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

	if (e->Type() == EVENT_FORM_INTERACTION && e->Forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->Forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
		else if (e->Forms().RaisedBy->Name == newSaveItemId)
		{
			if (saveManager.newSaveGame("new_save_game", currentState))
			{
				stageCmd.cmd = StageCmd::Command::POP;
			}
			return;
		}
		else if (e->Forms().RaisedBy->Name == existingSaveItemId)
		{
			if (currentAction == Load)
			{
				sp<SaveMetadata> slot = e->Forms().RaisedBy->GetData<SaveMetadata>();

				stageCmd.nextStage = mksp<LoadingScreen>(std::move(saveManager.loadGame(*slot)));
				stageCmd.cmd = StageCmd::Command::PUSH;
				return;
			}
			else
			{
				sp<SaveMetadata> slot = e->Forms().RaisedBy->GetData<SaveMetadata>();
				if (saveManager.overrideGame(*slot, currentState))
				{
					stageCmd.cmd = StageCmd::Command::POP;
					return;
				}
			}
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
