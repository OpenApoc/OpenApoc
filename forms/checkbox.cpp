
#include "checkbox.h"
#include "../framework/framework.h"
#include "../game/resources/gamecore.h"

namespace OpenApoc {

RawSound* CheckBox::buttonclick = nullptr;

CheckBox::CheckBox( Control* Owner ) : Control( Owner ), Checked(false), imagechecked(nullptr), imageunchecked(nullptr)
{
	LoadResources();
	if( buttonclick == nullptr )
	{
		buttonclick = new RawSound( "STRATEGC/INTRFACE/BUTTON1.RAW" );
	}
}

CheckBox::~CheckBox()
{
}

void CheckBox::LoadResources()
{
	if( !imagechecked )
	{
		imagechecked = GAMECORE->GetImage( "PCK:UFODATA/NEWBUT.PCK:UFODATA/NEWBUT.TAB:65:UI/UI_PALETTE.PNG" );
		if( Size.x == 0 )
		{
			Size.x = imagechecked->width;
		}
		if( Size.y == 0 )
		{
			Size.y = imagechecked->height;
		}
	}
	if( !imageunchecked )
	{
		imageunchecked = GAMECORE->GetImage( "PCK:UFODATA/NEWBUT.PCK:UFODATA/NEWBUT.TAB:64:UI/UI_PALETTE.PNG" );
	}
}

void CheckBox::EventOccured( Event* e )
{
	Control::EventOccured( e );

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this && e->Data.Forms.EventFlag == FormEventType::MouseDown )
	{
		buttonclick->PlaySound();
	}

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this && e->Data.Forms.EventFlag == FormEventType::MouseClick )
	{
		Checked = !Checked;
		Event* ce = new Event();
		ce->Type = e->Type;
		memcpy( (void*)&(ce->Data.Forms), (void*)&(e->Data.Forms), sizeof( FRAMEWORK_FORMS_EVENT ) );
		ce->Data.Forms.EventFlag = FormEventType::CheckBoxChange;
		FRAMEWORK->PushEvent( ce );
	}
}

void CheckBox::OnRender()
{
	std::shared_ptr<Image> useimage;

	LoadResources();

	useimage = ( Checked ? imagechecked : imageunchecked);

	if( useimage != nullptr )
	{
		int bmpw = useimage->width;
		int bmph = useimage->height;
		if( bmpw == Size.x && bmph == Size.y )
		{
			useimage->draw( 0, 0 );
		} else {
			useimage->drawScaled(0, 0, bmpw, bmph, 0, 0, this->Size.x, this->Size.y);
		}
	}
}

void CheckBox::Update()
{
	Control::Update();
}

void CheckBox::UnloadResources()
{
	imagechecked.reset();
	imageunchecked.reset();
	Control::UnloadResources();
}

}; //namespace OpenApoc
