#include "game/ui/ufopaedia/ufopaediacategoryview.h"
#include "forms/forms.h"
#include "forms/ui.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "library/sp.h"

namespace OpenApoc
{

UfopaediaCategoryView::UfopaediaCategoryView(GameState &state, sp<UfopaediaCategory> cat)
    : Stage(), menuform(ui().GetForm("FORM_UFOPAEDIA_BASE")), state(state), category(cat)
{
}

UfopaediaCategoryView::~UfopaediaCategoryView() {}

void UfopaediaCategoryView::Begin()
{
	auto infoLabel = menuform->FindControlTyped<Label>("TEXT_INFO");
	auto entryList = menuform->FindControlTyped<ListBox>("LISTBOX_SHORTCUTS");
	entryList->Clear();
	entryList->ItemSize = infoLabel->GetFont()->GetFontHeight() + 2;
	for (auto &pair : this->category->entries)
	{
		auto entry = pair.second;
		// Skip non-visible entries
		if (!entry->isVisible())
		{
			continue;
		}

		auto entryControl = mksp<TextButton>(tr(entry->title), infoLabel->GetFont());
		entryControl->Name = "ENTRY_SHORTCUT";
		entryControl->RenderStyle = TextButton::ButtonRenderStyle::Flat;
		entryControl->TextHAlign = HorizontalAlignment::Left;
		entryControl->TextVAlign = VerticalAlignment::Centre;
		entryControl->SetData(entry);
		entryList->AddItem(entryControl);
	}
	// Start with the intro page
	this->position_iterator = this->category->entries.end();
	this->setFormData();
}

void UfopaediaCategoryView::Pause() {}

void UfopaediaCategoryView::Resume() {}

void UfopaediaCategoryView::Finish() {}

void UfopaediaCategoryView::EventOccurred(Event *e)
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
		if (e->Forms().RaisedBy->Name == "BUTTON_NEXT_TOPIC")
		{
			do
			{
				if (this->position_iterator == this->category->entries.end())
				{
					this->position_iterator = this->category->entries.begin();
				}
				else
				{
					this->position_iterator++;
				}
				// Loop until we find the end (which shows the category intro screen)
				// or a visible entry
			} while (this->position_iterator != this->category->entries.end() &&
			         !this->position_iterator->second->isVisible());
			this->setFormData();
			return;
		}
		if (e->Forms().RaisedBy->Name == "BUTTON_PREVIOUS_TOPIC")
		{
			do
			{
				if (this->position_iterator == this->category->entries.begin())
				{
					this->position_iterator = this->category->entries.end();
				}
				else
				{
					this->position_iterator--;
				}
				// Loop until we find the end (which shows the category intro screen)
				// or a visible entry
			} while (this->position_iterator != this->category->entries.end() &&
			         !this->position_iterator->second->isVisible());
			this->setFormData();
			return;
		}
		if (e->Forms().RaisedBy->Name == "BUTTON_NEXT_SECTION")
		{
			auto it = state.ufopaedia.begin();
			// First find myself
			while (it->second != this->category)
			{
				it++;
				if (it == state.ufopaedia.end())
				{
					LogError("Failed to find current category \"%s\"",
					         this->category->title.c_str());
				}
			}
			// Increment it once to get the next
			it++;
			// Loop around to the beginning
			if (it == state.ufopaedia.end())
			{
				it = state.ufopaedia.begin();
			}
			stageCmd.cmd = StageCmd::Command::REPLACE;
			stageCmd.nextStage = mksp<UfopaediaCategoryView>(state, it->second);
			return;
		}
		if (e->Forms().RaisedBy->Name == "BUTTON_PREVIOUS_SECTION")
		{
			auto it = state.ufopaedia.begin();
			// First find myself
			while (it->second != this->category)
			{
				it++;
				if (it == state.ufopaedia.end())
				{
					LogError("Failed to find current category \"%s\"",
					         this->category->title.c_str());
				}
			}
			// Loop around to the beginning
			if (it == state.ufopaedia.begin())
			{
				it = state.ufopaedia.end();
			}
			// Decrement it once to get the previous
			it--;
			stageCmd.cmd = StageCmd::Command::REPLACE;
			stageCmd.nextStage = mksp<UfopaediaCategoryView>(state, it->second);
			return;
		}
		if (e->Forms().RaisedBy->Name == "ENTRY_SHORTCUT")
		{
			auto entry = e->Forms().RaisedBy->GetData<UfopaediaEntry>();
			if (!entry)
			{
				LogError("Invalid UfopaediaEntry in shortcut control");
			}
			auto it = this->category->entries.begin();
			// Find the entry iterator
			while (it->second != entry)
			{
				it++;
				if (it == this->category->entries.end())
				{
					LogError("Failed to find current category \"%s\"",
					         this->category->title.c_str());
				}
			}
			this->position_iterator = it;
			this->setFormData();
		}
		if (e->Forms().RaisedBy->Name == "BUTTON_INFORMATION")
		{
			menuform->FindControl("INFORMATION_PANEL")->Visible =
			    !menuform->FindControl("INFORMATION_PANEL")->Visible;
			return;
		}
	}
}

void UfopaediaCategoryView::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void UfopaediaCategoryView::Render()
{
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	// fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});
	menuform->Render();
}

void UfopaediaCategoryView::setFormData()
{
	UString title;
	UString description;
	sp<Image> background;
	if (this->position_iterator == this->category->entries.end())
	{
		title = category->title;
		description = category->description;
		background = category->background;
	}
	else
	{
		title = this->position_iterator->second->title;
		description = this->position_iterator->second->description;
		background = this->position_iterator->second->background;
	}
	menuform->FindControlTyped<Graphic>("BACKGROUND_PICTURE")->SetImage(background);
	auto tr_description = tr(description);
	auto tr_title = tr(title);
	menuform->FindControlTyped<Label>("TEXT_INFO")->SetText(tr_description);
	menuform->FindControlTyped<Label>("TEXT_TITLE_DATA")->SetText(tr_title);

	// Every time you we change the entry reset the info panel
	menuform->FindControl("INFORMATION_PANEL")->Visible = false;
}

bool UfopaediaCategoryView::IsTransition() { return false; }

}; // namespace OpenApoc
