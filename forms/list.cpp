
#include "list.h"

namespace OpenApoc {

ListBox::ListBox( Control* Owner ) : Control( Owner )
{
	ConfigureInternalScrollBar();
}

ListBox::ListBox( Control* Owner, VScrollBar* ExternalScrollBar ) : Control( Owner )
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
	scroller = new VScrollBar( this );
	scroller->Size.X = 16;
	scroller->Size.Y = 16;
	scroller->Location.X = this->Size.X - 16;
	scroller->Location.Y = 0;
	scroller_is_internal = true;
}

void ListBox::OnRender()
{
	if( scroller_is_internal )
	{
		scroller->Location.X = this->Size.X - scroller->Size.X;
		scroller->Size.Y = this->Size.Y;
	}

	int yoffset = 0;

	for( auto c = Controls.begin(); c != Controls.end(); c++ )
	{
		Control* ctrl = (Control*)*c;
		if( ctrl != scroller )
		{
			ctrl->Location.X = 0;
			ctrl->Location.Y = yoffset - scroller->Value;
			ctrl->Size.X = ( scroller_is_internal ? scroller->Location.X : this->Size.X );
			ctrl->Size.Y = 64;
			yoffset += ctrl->Size.Y + 1;
		}
	}
	scroller->Maximum = (yoffset - this->Size.Y);
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
