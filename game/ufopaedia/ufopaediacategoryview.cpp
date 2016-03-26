#include "game/ufopaedia/ufopaediacategoryview.h"
#include "forms/forms.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"
#include "library/sp.h"

namespace OpenApoc
{

UfopaediaCategoryView::UfopaediaCategoryView(GameState &state, sp<UfopaediaCategory> cat)
    : Stage(), menuform(fw().gamecore->GetForm("FORM_UFOPAEDIA_BASE")), state(state), category(cat)
{
	this->position_iterator = cat->entries.end();
	this->setFormData();
}

UfopaediaCategoryView::~UfopaediaCategoryView() {}

void UfopaediaCategoryView::Begin() {}

void UfopaediaCategoryView::Pause() {}

void UfopaediaCategoryView::Resume() {}

void UfopaediaCategoryView::Finish() {}

void UfopaediaCategoryView::EventOccurred(Event *e)
{
	menuform->EventOccured(e);
	fw().gamecore->MouseCursor->EventOccured(e);

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
		LogWarning("Click from \"%s\"", e->Forms().RaisedBy->Name.c_str());
		if (e->Forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			menuform->FindControl("INFORMATION_PANEL")->Visible = false;
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
		if (e->Forms().RaisedBy->Name == "BUTTON_NEXT_TOPIC")
		{
			LogWarning("Next topic");
			if (this->position_iterator == this->category->entries.end())
			{
				this->position_iterator = this->category->entries.begin();
			}
			else
			{
				this->position_iterator++;
			}
			this->setFormData();
			return;
		}
		if (e->Forms().RaisedBy->Name == "BUTTON_PREVIOUS_TOPIC")
		{
			LogWarning("Prev topic");
			if (this->position_iterator == this->category->entries.begin())
			{
				this->position_iterator = this->category->entries.end();
			}
			else
			{
				this->position_iterator--;
			}
			this->setFormData();
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
	fw().gamecore->MouseCursor->Render();
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
		LogWarning("Showing ufopaedia category \"%s\"", title.c_str());
	}
	else
	{
		title = this->position_iterator->second->title;
		description = this->position_iterator->second->description;
		background = this->position_iterator->second->background;
		LogWarning("Showing ufopaedia entry \"%s\"", title.c_str());
	}
	menuform->FindControlTyped<Graphic>("BACKGROUND_PICTURE")->SetImage(background);
	auto tr_description = tr(description);
	LogInfo("description = \"%s\"", tr_description.c_str());
	auto tr_title = tr(title);
	menuform->FindControlTyped<Label>("TEXT_INFO")->SetText(tr_description);
	menuform->FindControlTyped<Label>("TEXT_TITLE_DATA")->SetText(tr_title);
}

bool UfopaediaCategoryView::IsTransition() { return false; }

}; // namespace OpenApoc
