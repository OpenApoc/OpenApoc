#pragma once
#include "game/ui/base/transactionscreen.h"
#include "library/sp.h"

namespace OpenApoc
{

class BuyAndSellScreen : public TransactionScreen
{
  private:
	//
	int getLeftIndex() override;
	int getRightIndex() override;
	//
	void updateFormValues(bool queueHighlightUpdate = true);
	//
	void closeScreen() override;
	// Execute orders given in the screen
	void executeOrders() override;

  public:
	BuyAndSellScreen(sp<GameState> state, bool forceLimits = false);
	~BuyAndSellScreen() override = default;
};
}
