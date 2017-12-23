#pragma once
#include "game/ui/base/transactionscreen.h"
#include "library/sp.h"

namespace OpenApoc
{

class Base;
class GameState;

class TransactionTransfer : public TransactionScreen
{
  private:
	StateRef<Base> second_base;
	sp<GraphicButton> currentSecondView;
	sp<Label> textViewSecondBaseStatic;

	//
	void changeSecondBase(sp<Base> newBase);
	//
	int getRightIndex() override;
	//
	void updateBaseHighlight() override;
	//
	virtual void closeScreen(bool forced = false) override;
	// Execute orders given in the screen
	virtual void executeOrders() override;
	//
	void initViewSecondBase() override;

  public:
	TransactionTransfer(sp<GameState> state, bool forceLimits = false);
	~TransactionTransfer() = default;

	// Stage control
	void render() override;
};
}
