#pragma once

#include "library/observer.h"
#include "library/sp.h"
#include "library/strings.h"
#include <functional>
#include <list>
#include <unordered_map>

namespace OpenApoc
{

/**
 * The ObservableLinkedMap contains map of items and the order of items insertion.
 * Also the class broadcasts events for observers.
 * The class is not multithread safe.
 */
template <class T> class ObservableLinkedMap : public Observable
{
  public:
	ObservableLinkedMap() = default;
	virtual ~ObservableLinkedMap();

	// Get the map of items.
	const std::unordered_map<UString, sp<T>> &getMap() const { return map; }
	// Get the list of items.
	const std::list<sp<T>> &getList() const { return list; }
	// Get ID of the item.
	const UString &getId(sp<T> item) const;
	// Get item by ID.
	sp<T> getItem(const UString &id) const;

	// Add item to the linked map.
	void addItem(const UString &id, sp<T> item);
	// Remove item from the linked map.
	void removeItem(const UString &id);
	// Remove item from the linked map.
	void removeItem(sp<T> item);
	// Change item in the linked map.
	void changeItem(sp<T> item);

	// Change the order of items in the list.
	void sort(std::function<bool(const sp<T> &a, const sp<T> &b)> comp) { list.sort(comp); }

  protected:
	// Broadcast the "add item" event.
	void fireAddEvent(sp<T> &item);
	// Broadcast the "remove item" event.
	void fireRemoveEvent(sp<T> &item);
	// Broadcast the "change item" event.
	void fireChangeEvent(sp<T> &item);

	// The map of IDs and items.
	std::unordered_map<UString, sp<T>, UString::Hash> map;
	// The list contains the order of items insertion.
	std::list<sp<T>> list;
};

} // namespace OpenApoc