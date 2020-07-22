#pragma once
#include "game/ui/base/transactionscreen.h"
#include "library/sp.h"

namespace OpenApoc
{

class TransferScreen : public TransactionScreen
{
  private:
	StateRef<Base> second_base;
	sp<GraphicButton> currentSecondView;

	sp<Label> textViewSecondBase;
	sp<Label> textViewSecondBaseStatic;

	std::vector<sp<Image>> bigUnitRanks;

	// Change the base on the right side.
	void changeSecondBase(sp<Base> newBase);
	// Get the right side index.
	int getRightIndex() override;
	// Update highlight of facilities on the mini-view.
	void updateBaseHighlight() override;
	void displayItem(sp<TransactionControl> control) override;
	// Checking conditions and limitations before the execution of orders.
	void closeScreen() override;
	// Execute orders given in the screen.
	void executeOrders() override;
	// Init mini-view of the second base.
	void initViewSecondBase() override;

  public:
	TransferScreen(sp<GameState> state, bool forceLimits = false);
	~TransferScreen() override = default;

	// Stage control
	void render() override;
};

}; // namespace OpenApoc
