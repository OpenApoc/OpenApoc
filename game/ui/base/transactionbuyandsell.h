#pragma once
#include "game/ui/base/transactionscreen.h"
#include "library/sp.h"

namespace OpenApoc
{

class GameState;

class TransactionBuyAndSell : public TransactionScreen
{
  private:
	//
	int getLeftIndex() override;
	int getRightIndex() override;
	//
	void updateFormValues(bool queueHighlightUpdate = true);
	//
	virtual void closeScreen(bool forced = false) override;
	// Execute orders given in the screen
	virtual void executeOrders() override;

  public:
	TransactionBuyAndSell(sp<GameState> state, bool forceLimits = false);
	~TransactionBuyAndSell() = default;
};
}
