#include "game/state/observablelinkedmap.h"
#include "game/state/observerlinkedmap.h"

namespace OpenApoc
{

template <class T> ObservableLinkedMap<T>::~ObservableLinkedMap()
{
	for (auto &i : map)
	{
		fireRemoveEvent(i.second);
	}
	list.clear();
	map.clear();
}

/**
 * Get ID of the item.
 */
template <class T> const UString &ObservableLinkedMap<T>::getId(sp<T> item) const
{
	for (auto &i : map)
	{
		if (i.second == item)
			return i.first;
	}
	return nullptr;
}

/**
 * Get item by ID.
 */
template <class T> sp<T> ObservableLinkedMap<T>::getItem(const UString &id) const
{
	auto it = map.find(id);
	return it == map.end() ? nullptr : it->second;
}

/**
 * Add item to the linked map.
 */
template <class T> void ObservableLinkedMap<T>::addItem(const UString &id, sp<T> item)
{
	auto it = map.find(id);
	if (it == map.end())
	{
		fireAddEvent(item);
		list.push_back(item);
		map.emplace(id, item);
	}
	else if (it->second == item)
	{
		fireChangeEvent(item);
	}
	else // found, but the item is not the same
	{
		removeItem(id);
		addItem(id, item);
	}
}

/**
 * Remove item from the linked map.
 */
template <class T> void ObservableLinkedMap<T>::removeItem(const UString &id)
{
	auto it = map.find(id);
	if (it != map.end())
	{
		fireRemoveEvent(it->second);
		list.remove(it->second);
		map.erase(id);
	}
}

/**
 * Remove item from the linked map.
 */
template <class T> void ObservableLinkedMap<T>::removeItem(sp<T> item)
{
	for (auto &i : map)
	{
		if (i.second == item)
		{
			fireRemoveEvent(i.second);
			list.remove(i.second);
			map.erase(i.first);
			break;
		}
	}
}

/**
 * Change item in the linked map.
 * @param item - the item that has changed
 */
template <class T> void ObservableLinkedMap<T>::changeItem(sp<T> item)
{
	// Assume that the item exist in the map.
	// TODO: add either a check or a conditional check of existence in the map
	fireChangeEvent(item);
}

/**
 * Broadcast the "add item" event.
 * @param item - added item
 */
template <class T> void ObservableLinkedMap<T>::fireAddEvent(sp<T> &item)
{
	for (auto observer : observers)
	{
		static_cast<ObserverLinkedMap<T> *>(observer)->added(item);
	}
}

/**
 * Broadcast the "remove item" event.
 * @param item - removed item
 */
template <class T> void ObservableLinkedMap<T>::fireRemoveEvent(sp<T> &item)
{
	for (auto observer : observers)
	{
		static_cast<ObserverLinkedMap<T> *>(observer)->removed(item);
	}
}

/**
 * Broadcast the "change item" event.
 * @param item - changed item
 */
template <class T> void ObservableLinkedMap<T>::fireChangeEvent(sp<T> &item)
{
	for (auto observer : observers)
	{
		static_cast<ObserverLinkedMap<T> *>(observer)->changed(item);
	}
}

} // namespace OpenApoc