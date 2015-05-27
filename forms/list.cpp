
#include "forms/list.h"
#include "forms/vscrollbar.h"

namespace OpenApoc {

ListBox::ListBox( Framework &fw, Control* Owner ) : Control( fw, Owner )
{
	ConfigureInternalScrollBar();
}

ListBox::ListBox( Framework &fw, Control* Owner, VScrollBar* ExternalScrollBar ) : Control( fw, Owner )
{
	if( ExternalScrollBar == nullptr )
	{
		ConfigureInternalScrollBar();
	} else {
		scroller = ExternalScrollBar;
		scroller_is_internal = false;
	}
}

ListBox::~ListBox()
{
	Clear();
}

void ListBox::ConfigureInternalScrollBar()
{
	scroller = new VScrollBar( fw, this );
	scroller->Size.x = 16;
	scroller->Size.y = 16;
	scroller->Location.x = this->Size.x - 16;
	scroller->Location.y = 0;
	scroller_is_internal = true;
}

void ListBox::OnRender()
{
	if( scroller_is_internal )
	{
		scroller->Location.x = this->Size.x - scroller->Size.x;
		scroller->Size.y = this->Size.y;
	}

	int yoffset = 0;

	for( auto c = Controls.begin(); c != Controls.end(); c++ )
	{
		Control* ctrl = (Control*)*c;
		if( ctrl != scroller )
		{
			ctrl->Location.x = 0;
			ctrl->Location.y = yoffset - scroller->Value;
			ctrl->Size.x = ( scroller_is_internal ? scroller->Location.x : this->Size.x );
			ctrl->Size.y = 64;
			yoffset += ctrl->Size.y + 1;
		}
	}
	scroller->Maximum = (yoffset - this->Size.y);
	scroller->LargeChange = Maths::Max( (scroller->Maximum - scroller->Minimum + 2) / 10.0f, 4.0f );

}

void ListBox::EventOccured( Event* e )
{
	Control::EventOccured( e );
}

void ListBox::Update()
{
}

void ListBox::UnloadResources()
{
}

void ListBox::Clear()
{
	while( Controls.size() > 0 )
	{
		delete Controls.back();
		Controls.pop_back();
	}
}

void ListBox::AddItem( Control* Item )
{
	Controls.push_back( Item );
}

Control* ListBox::RemoveItem( Control* Item )
{
	for( auto i = Controls.begin(); i != Controls.end(); i++ )
	{
		if( (Control*)*i == Item )
		{
			Controls.erase( i );
			return Item;
		}
	}
	return nullptr;
}

Control* ListBox::RemoveItem( int Index )
{
	Control* c = Controls.at(Index);
	Controls.erase( Controls.begin() + Index );
	return c;
}

Control* ListBox::operator[]( int Index )
{
	return Controls.at(Index);
}

}; //namespace OpenApoc
