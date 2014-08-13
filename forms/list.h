
#pragma once

#include "control.h"
#include "vscrollbar.h"

class ListBox : public Control
{
	private:
		// std::vector<Control*> items;
		VScrollBar* scroller;
		bool scroller_is_internal;

		void ConfigureInternalScrollBar();

	protected:
		virtual void OnRender();

	public:
		ListBox( Control* Owner );
		ListBox( Control* Owner, VScrollBar* ExternalScrollBar );
		virtual ~ListBox();

		virtual void EventOccured( Event* e );
		virtual void Update();
		virtual void UnloadResources();

		void Clear();
		void AddItem( Control* Item );
		Control* RemoveItem( Control* Item );
		Control* RemoveItem( int Index );
		Control* operator[]( int Index );
};