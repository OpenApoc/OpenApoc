#pragma once

#include "library/observer.h"
#include "library/sp.h"

namespace OpenApoc
{

template <class T> class ObserverLinkedMap : public Observer
{
  public:
	// The "added" event occurs when the item added to the linked map.
	virtual void added(sp<T> &item) = 0;
	// The "removed" event occurs when the item removed from the linked map.
	virtual void removed(sp<T> &item) = 0;
	// The "changed" event occurs when the item changed in the linked map.
	virtual void changed(sp<T> &item) = 0;
};

} // namespace OpenApoc