#pragma once
#include "game/ui/base/transactionscreen.h"
#include "library/sp.h"

namespace OpenApoc
{

class AlienContainmentScreen : public TransactionScreen
{
  private:
	void closeScreen() override;
	void executeOrders() override;

  public:
	AlienContainmentScreen(sp<GameState> state, bool forceLimits = false);
	~AlienContainmentScreen() override = default;
};

}; // namespace OpenApoc
