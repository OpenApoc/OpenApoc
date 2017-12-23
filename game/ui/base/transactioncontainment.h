#pragma once
#include "game/ui/base/transactionscreen.h"
#include "library/sp.h"

namespace OpenApoc
{

class GameState;

class TransactionContainment : public TransactionScreen
{
  private:
	//
	virtual void closeScreen(bool forced = false) override;
	// Execute orders given in the screen
	virtual void executeOrders() override;

  public:
	TransactionContainment(sp<GameState> state, bool forceLimits = false);
	~TransactionContainment() = default;
};
}
