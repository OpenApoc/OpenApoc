
#include "ufopaediacategory.h"
#include "framework/framework.h"

namespace OpenApoc {

UfopaediaCategory::UfopaediaCategory(Framework &fw, tinyxml2::XMLElement* Element) : Stage(fw)
{
	UString nodename;

	ViewingEntry = 0;

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
	Label* infolabel = ((Label*)menuform->FindControl("TEXT_INFO"));
	ListBox* entrylist = ((ListBox*)menuform->FindControl("LISTBOX_SHORTCUTS"));
	entrylist->Clear();
	entrylist->ItemHeight = infolabel->GetFont()->GetFontHeight() + 2;
	int idx = 1;
	for( auto entry = Entries.begin(); entry != Entries.end(); entry++ )
	{
		std::shared_ptr<UfopaediaEntry> e = (std::shared_ptr<UfopaediaEntry>)*entry;
		TextButton* tb = new TextButton( fw, nullptr, fw.gamecore->GetString(e->Title), infolabel->GetFont() );
		tb->Name = "Index" + Strings::FromInteger( idx );
		tb->RenderStyle = TextButton::TextButtonRenderStyles::SolidButtonStyle;
		tb->TextHAlign = HorizontalAlignment::Left;
		tb->TextVAlign = VerticalAlignment::Centre;
		tb->BackgroundColour.a = 0;
		entrylist->AddItem( tb );
		idx++;
	}

	SetupForm();
}

void UfopaediaCategory::Pause()
{
}

void UfopaediaCategory::Resume()
{
}

void UfopaediaCategory::Finish()
{
	//ListBox* entrylist = ((ListBox*)menuform->FindControl("LISTBOX_SHORTCUTS"));
	//entrylist->Clear();
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
			menuform->FindControl("INFORMATION_PANEL")->Visible = false;
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
		else if( e->Data.Forms.RaisedBy->Name == "BUTTON_INFORMATION" )
		{
			menuform->FindControl("INFORMATION_PANEL")->Visible = !menuform->FindControl("INFORMATION_PANEL")->Visible;
		}
		else if( e->Data.Forms.RaisedBy->Name == "BUTTON_NEXT_SECTION" )
		{
			menuform->FindControl("INFORMATION_PANEL")->Visible = false;
			return;
		}
		else if( e->Data.Forms.RaisedBy->Name == "BUTTON_NEXT_TOPIC" )
		{
			menuform->FindControl("INFORMATION_PANEL")->Visible = false;
			if( ViewingEntry < Entries.size() )
			{
				SetTopic( ViewingEntry + 1 );
			}
			return;
		}
		else if( e->Data.Forms.RaisedBy->Name == "BUTTON_PREVIOUS_TOPIC" )
		{
			menuform->FindControl("INFORMATION_PANEL")->Visible = false;
			if( ViewingEntry > 0 )
			{
				SetTopic( ViewingEntry - 1 );
			}
			return;
		}
		else if( e->Data.Forms.RaisedBy->Name == "BUTTON_PREVIOUS_SECTION" )
		{
			menuform->FindControl("INFORMATION_PANEL")->Visible = false;
			return;
		}
		else if( e->Data.Forms.RaisedBy->Name.substr( 0, 5 ) == "Index" )
		{
			UString nameidx = e->Data.Forms.RaisedBy->Name.substr( 5, e->Data.Forms.RaisedBy->Name.length() - 5 );
			int idx = Strings::ToInteger( nameidx );
			SetTopic( idx );
			menuform->FindControl("INFORMATION_PANEL")->Visible = false;
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

void UfopaediaCategory::SetTopic(int Index)
{
	ViewingEntry = Index;
	SetupForm();
}

void UfopaediaCategory::SetupForm()
{
	if( ViewingEntry == 0 )
	{
		((Graphic*)menuform->FindControl("BACKGROUND_PICTURE"))->SetImage( fw.data->load_image(BackgroundImageFilename) );
		Label* infolabel = ((Label*)menuform->FindControl("TEXT_INFO"));
		infolabel->SetText( fw.gamecore->GetString(BodyInformation) );
		infolabel = ((Label*)menuform->FindControl("TEXT_TITLE_DATA"));
		infolabel->SetText( fw.gamecore->GetString(Title).toUpper() );
	} else {
		std::shared_ptr<UfopaediaEntry> e = Entries.at( ViewingEntry - 1 );
		((Graphic*)menuform->FindControl("BACKGROUND_PICTURE"))->SetImage( fw.data->load_image( e->BackgroundImageFilename ) );
		Label* infolabel = ((Label*)menuform->FindControl("TEXT_INFO"));
		infolabel->SetText( fw.gamecore->GetString( e->BodyInformation ) );
		infolabel = ((Label*)menuform->FindControl("TEXT_TITLE_DATA"));
		infolabel->SetText( fw.gamecore->GetString( e->Title ).toUpper() );
	}
}

}; //namespace OpenApoc
