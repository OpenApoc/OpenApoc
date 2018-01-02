#pragma once
#include "game/ui/base/transactionscreen.h"
#include "library/sp.h"

namespace OpenApoc
{

class BuyAndSellScreen : public TransactionScreen
{
  private:
	// Get the left side index.
	int getLeftIndex() override;
	// Get the right side index.
	int getRightIndex() override;
	// Update statistics on TransactionControls.
	void updateFormValues(bool queueHighlightUpdate = true) override;
	// Checking conditions and limitations before the execution of orders.
	void closeScreen() override;
	// Execute orders given in the screen.
	void executeOrders() override;

  public:
	BuyAndSellScreen(sp<GameState> state, bool forceLimits = false);
	~BuyAndSellScreen() override = default;
};

}; // namespace OpenApoc
