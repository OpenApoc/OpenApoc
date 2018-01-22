#include "library/observer.h"

namespace OpenApoc
{

Observable::Observable(Observable &&observable)
{
	for (Observer *observer : observable.observers)
	{
		observable.detach(observer);
		attach(observer);
	}
}

Observable::~Observable()
{
	for (Observer *observer : observers)
	{
		detach(observer);
	}
}

Observable &Observable::operator=(Observable &&observable)
{
	for (Observer *observer : observable.observers)
	{
		observable.detach(observer);
		attach(observer);
	}
	return *this;
}

/**
 * Add observer to the observers list.
 * @param observer - the observer that should be added to the list.
 */
void Observable::attach(Observer *observer)
{
	observer->attached(this);
	observers.push_back(observer);
}

/**
 * Remove observer from the observer list.
 * @param observer - the observer that should be removed from the list.
 */
void Observable::detach(Observer *observer)
{
	observer->detached();
	observers.remove(observer);
}

Observer::Observer(const Observer &observer)
{
	if (observer.observable)
		observer.observable->attach(this);
}

Observer::Observer(Observer &&observer)
{
	if (Observable *observable = observer.observable)
	{
		observable->detach(&observer);
		observable->attach(this);
	}
}

Observer::~Observer()
{
	if (observable)
		observable->detach(this);
}

Observer &Observer::operator=(const Observer &observer)
{
	if (observer.observable)
		observer.observable->attach(this);
	return *this;
}

Observer &Observer::operator=(Observer &&observer)
{
	if (Observable *observable = observer.observable)
	{
		observable->detach(&observer);
		observable->attach(this);
	}
	return *this;
}

/**
 * The attached event.
 * @param observable - that attached this observer.
 */
void Observer::attached(Observable *observable)
{
	if (this->observable)
		this->observable->detach(this);
	this->observable = observable;
}

/**
 * The detached event.
 */
void Observer::detached() { observable = nullptr; }

} // namespace OpenApoc