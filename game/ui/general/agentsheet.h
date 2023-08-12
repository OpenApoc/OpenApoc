#include "forms/form.h"
#include "game/state/shared/agent.h"
#include "library/sp.h"

namespace OpenApoc
{

/*
 * Implements some static methods to display the stat sheet for an agent equipment item/itemtype
 * To be used in agent equipment and transaction screens (maybe ufopaedia as well?)
 */
class AgentSheet
{
  public:
	AgentSheet(sp<Form> profileForm, sp<Form> statsForm);
	AgentSheet(sp<GameState> state, sp<Form> profileForm, sp<Form> statsForm, sp<Form> historyFrom);
	void display(const Agent &item, std::vector<sp<Image>> &ranks, bool turnBased = true);
	void clear();

  private:
	sp<Form> profileForm;
	sp<Form> historyForm;
	sp<Form> statsForm;
	sp<GameState> state;
	sp<Image> createStatsBar(int initialValue, int currentValue, int modifiedValue, int maxValue,
	                         const std::pair<Colour, Colour> &colour, Vec2<int> imageSize);

	void displayProfile(const Agent &item, std::vector<sp<Image>> &ranks);
	void displayHistory(const Agent &item);
	void displayStats(const Agent &item, std::vector<sp<Image>> &ranks, bool turnBased = true);
};

}; // namespace OpenApoc
