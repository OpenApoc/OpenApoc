
#include "ufopaediacategory.h"
#include "framework/framework.h"

namespace OpenApoc {

UfopaediaCategory::UfopaediaCategory(Framework &fw, tinyxml2::XMLElement* Element) : Stage(fw)
{
	UString nodename;

	if( Element->Attribute("id") != nullptr && UString(Element->Attribute("id")) != "" )
	{
		ID = Element->Attribute("id");
	}

	tinyxml2::XMLElement* node;
	for( node = Element->FirstChildElement(); node != nullptr; node = node->NextSiblingElement() )
	{
		nodename = node->Name();

		if( nodename == "backgroundimage" )
		{
			BackgroundImageFilename = node->GetText();
		}
		if( nodename == "title" )
		{
			Title = node->GetText();
		}
		if( nodename == "text_info" )
		{
			BodyInformation = node->GetText();
		}
		if( nodename == "entries" )
		{
			tinyxml2::XMLElement* node2;
			for( node2 = node->FirstChildElement(); node2 != nullptr; node2 = node2->NextSiblingElement() )
			{
				std::shared_ptr<UfopaediaEntry> newentry = std::make_shared<UfopaediaEntry>( node2 );
				Entries.push_back( newentry );
			}
		}
	}

	menuform = fw.gamecore->GetForm("FORM_UFOPAEDIA_BASE");


}

UfopaediaCategory::~UfopaediaCategory()
{
}

void UfopaediaCategory::Begin()
{
	((Graphic*)menuform->FindControl("BACKGROUND_PICTURE"))->SetImage( fw.data->load_image(BackgroundImageFilename) );
	Label* infolabel = ((Label*)menuform->FindControl("TEXT_INFO"));
	infolabel->SetText( fw.gamecore->GetString(BodyInformation) );
	ListBox* entrylist = ((ListBox*)menuform->FindControl("LISTBOX_SHORTCUTS"));
	entrylist->Clear();

	for( auto entry = Entries.begin(); entry != Entries.end(); entry++ )
	{
		std::shared_ptr<UfopaediaEntry> e = (std::shared_ptr<UfopaediaEntry>)*entry;
		TextButton* tb = new TextButton( fw, entrylist, fw.gamecore->GetString(e->Title), infolabel->GetFont() );
		entrylist->AddItem( tb );
	}

}

void UfopaediaCategory::Pause()
{
}

void UfopaediaCategory::Resume()
{
}

void UfopaediaCategory::Finish()
{
	ListBox* entrylist = ((ListBox*)menuform->FindControl("LISTBOX_SHORTCUTS"));
	entrylist->Clear();
}

void UfopaediaCategory::EventOccurred(Event *e)
{
	menuform->EventOccured( e );
	fw.gamecore->MouseCursor->EventOccured( e );

	if( e->Type == EVENT_KEY_DOWN )
	{
		if( e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE )
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick )
	{
		if( e->Data.Forms.RaisedBy->Name == "BUTTON_QUIT" )
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
		else if( e->Data.Forms.RaisedBy->Name == "BUTTON_INFORMATION" )
		{
			menuform->FindControl("INFORMATION_PANEL")->Visible = !menuform->FindControl("INFORMATION_PANEL")->Visible;
		}
		else if( e->Data.Forms.RaisedBy->Name == "BUTTON_NEXT_SECTION" )
		{
			return;
		}
		else if( e->Data.Forms.RaisedBy->Name == "BUTTON_NEXT_TOPIC" )
		{
			return;
		}
		else if( e->Data.Forms.RaisedBy->Name == "BUTTON_PREVIOUS_TOPIC" )
		{
			return;
		}
		else if( e->Data.Forms.RaisedBy->Name == "BUTTON_PREVIOUS_SECTION" )
		{
			return;
		}
		else
		{
			return;
		}
	}
}

void UfopaediaCategory::Update(StageCmd * const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	//Reset the command to default
	this->stageCmd = StageCmd();
}

void UfopaediaCategory::Render()
{
	fw.Stage_GetPrevious(this->shared_from_this())->Render();
	fw.renderer->drawFilledRect({0,0}, fw.Display_GetSize(), Colour{0,0,0,128});
	menuform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool UfopaediaCategory::IsTransition()
{
	return false;
}


}; //namespace OpenApoc
