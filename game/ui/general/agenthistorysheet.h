#include "forms/form.h"
#include "game/state/shared/agent.h"

namespace OpenApoc
{

class AgentHistorySheet
{
	sp<Form> historyForm;
	sp<GameState> state;

  public:
	AgentHistorySheet(sp<Form> historyForm, sp<GameState> gameState);
	void display(const Agent &item);
};

}; // namespace OpenApoc
