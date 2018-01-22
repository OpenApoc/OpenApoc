#pragma once

#include <list>

namespace OpenApoc
{

class Observer;

/**
 * The Observable class is a part of the Observer pattern.
 * The class broadcasts events that occurs in it.
 */
class Observable
{
  public:
	Observable() = default;
	Observable(Observable &&observable);
	virtual ~Observable();

	Observable &operator=(Observable &&observable);

	// Add observer to the observers list.
	virtual void attach(Observer *observer);
	// Remove observer from the observers list.
	virtual void detach(Observer *observer);

	// Broadcasts of events implements depends on the needs of derived class.

  protected:
	// The list of observers that should receive events.
	std::list<Observer *> observers;
};

/**
 * The Observer class
 * that receives events from an observable class.
 */
class Observer
{
  public:
	Observer() = default;
	Observer(const Observer &observer);
	Observer(Observer &&observer);
	virtual ~Observer();

	Observer &operator=(const Observer &observer);
	Observer &operator=(Observer &&observer);

	// The "attached" event occurs when the observer attached to an observable.
	virtual void attached(Observable *observable);
	// The "detached" event occurs when the observer detached from the observable.
	virtual void detached();

	// Other events implements depends on the needs of derived class.

  protected:
	// A class that broadcasts events.
	Observable *observable = nullptr;
};

} // namespace OpenApoc