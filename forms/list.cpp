
#include "list.h"

ListBox::ListBox( Control* Owner ) : Control( Owner ), scroller(nullptr)
{
}

ListBox::ListBox( Control* Owner, VScrollBar* ExternalScrollBar ) : Control( Owner ), scroller(ExternalScrollBar)
{
}

ListBox::~ListBox()
{
}

void ListBox::OnRender()
{
}

void ListBox::EventOccured( Event* e )
{
}

void ListBox::Update()
{
}

void ListBox::UnloadResources()
{
}


void ListBox::Clear()
{
	items.clear();
}

void ListBox::AddItem( Control* Item )
{
	items.push_back( Item );
}

Control* ListBox::RemoveItem( Control* Item )
{
	for( auto i = items.begin(); i != items.end(); i++ )
	{
		if( (Control*)*i == Item )
		{
			items.erase( i );
			return Item;
		}
	}
	return nullptr;
}

Control* ListBox::RemoveItem( int Index )
{
	Control* c = items.at(Index);
	items.erase( items.begin() + Index );
	return c;
}

Control* ListBox::operator[]( int Index )
{
	return items.at(Index);
}
