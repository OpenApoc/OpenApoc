#pragma once

#include "game/state/stateobject.h"
#include "library/colour.h"
#include "library/sp.h"
#include "library/vec.h"
#include <set>

namespace OpenApoc
{

class TileMap;

class SupportedMapPart
{
  public:
	// Attempts to re-link map parts to supports in the provided set
	static void attemptReLinkSupports(sp<std::set<SupportedMapPart *>> set);

	virtual ~SupportedMapPart() = default;

  public:
	// Compiles a list of parts supported by this part
	// Using sp because we switch to a new one constantly in re-linking
	// Using set because we need to easily weed out duplicates
	virtual sp<std::set<SupportedMapPart *>> getSupportedParts() = 0;

	// Makes mappart stop being valid for support and collapse in 1 vanilla tick
	virtual void queueCollapse(unsigned additionalDelay = 0) = 0;
	// Cancels queued collapse
	virtual void cancelCollapse() = 0;

	// Clear list of parts supported by this one
	virtual void clearSupportedParts() = 0;
	// Get ticks until part collapses
	virtual unsigned int getTicksUntilCollapse() const = 0;

	// Find map parts that support this one and set "hard supported" flag where appropriate
	virtual bool findSupport(bool allowClinging = true) = 0;

  protected:
	// Cease using support
	virtual void ceaseBeingSupported() = 0;

	//
	// For debug output
	//

	virtual Vec3<int> getTilePosition() const = 0;
	virtual const TileMap &getMap() const = 0;
	virtual UString getId() const = 0;
	virtual int getType() const = 0;
	virtual UString getSupportString() const = 0;
};

}; // namespace OpenApoc
