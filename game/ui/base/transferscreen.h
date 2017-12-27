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

	//
	void changeSecondBase(sp<Base> newBase);
	//
	int getRightIndex() override;
	//
	void updateBaseHighlight() override;
	//
	void closeScreen() override;
	// Execute orders given in the screen
	void executeOrders() override;
	//
	void initViewSecondBase() override;

  public:
	TransferScreen(sp<GameState> state, bool forceLimits = false);
	~TransferScreen() override = default;

	// Stage control
	void render() override;
};

}
