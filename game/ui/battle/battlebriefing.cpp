#include "game/ui/battle/battlebriefing.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlecommonimagelist.h"
#include "game/ui/battle/battleprestart.h"
#include "game/ui/tileview/battleview.h"
#include "game/ui/tileview/cityview.h"
#include <cmath>

namespace OpenApoc
{

namespace
{
static const std::map<UString, int> alienFunctionMap = {
    {"Alien Farm", 3},        {"Control Chamber", 2},   {"Dimension Gate Generator", 5},
    {"Food Chamber", 4},      {"Incubator Chamber", 6}, {"Maintenance Factory", 7},
    {"Megapod Chamber", 1},   {"Organic Factory", 8},   {"Sleeping Chamber", 9},
    {"Spawning Chamber", 10},
};
}

BattleBriefing::BattleBriefing(sp<GameState> state,
                               StateRef<Organisation> targetOrg [[maybe_unused]], UString location,
                               bool isBuilding, bool isRaid, std::shared_future<void> gameStateTask)
    : Stage(), menuform(ui().getForm("battle/briefing")), loading_task(std::move(gameStateTask)),
      state(state)
{

	menuform->findControlTyped<Label>("TEXT_DATE")
	    ->setText(format("%s      %s", state->gameTime.getLongDateString(),
	                     state->gameTime.getShortTimeString()));

	// FIXME: Read and store briefing text and image properly
	UString briefing = "";
	// Determine the type of mission and load appropriate briefing text and image.
	if (!isBuilding)
	{
		// Not a building: UFO assault.
		menuform->findControlTyped<Graphic>("BRIEFING_IMAGE")
		    ->setImage(fw().data->loadImage("xcom3/tacdata/brief3.pcx"));
		briefing = tr(""
			"You must attempt to capture the crashed UFO by eliminating the defending "
			"Alien forces. There is only one chance to succeed in this mission because "
			"failure will mean that the surviving Aliens will attempt to destroy their "
			"craft. They would rather die than allow us to recover their advanced "
			"technology. Your priority is to eliminate the enemy with maximum force.");
	}
	else
	{
		auto building = StateRef<Building>(&*state, location);
		if (building->base && building->owner == state->getPlayer())
		{
			// Building owned by the player: base defence.
			menuform->findControlTyped<Graphic>("BRIEFING_IMAGE")
			    ->setImage(fw().data->loadImage("xcom3/tacdata/brief4.pcx"));
			bool lastBase = state->player_bases.size() == 1;
			briefing = lastBase ? tr(""
									"Hostile troops have located your base and have launched an attack. Defend "
									"the base from costly damage. If all hostile forces are eliminated X-COM will"
									" be saved. If you are defeated then X-COM will be wiped out, leaving the "
									"world open to Alien domination. The fate of humanity will be determined by "
									"this conflict. Good luck.")
			                    : tr(""
									"Hostile troops have located your base and have launched an attack. Defend "
									"the base from costly damage by eliminating all invading forces. You must "
									"also safeguard your Scientists and Engineers, either by defending them or "
									"exiting them from the combat zone. You can retreat from the base to cut your"
									" losses, but the base will be lost.");
		}
		else if (!isRaid && building->owner != state->getAliens())
		{
			// Not a raid, building not owned by aliens: alien extermination/investigation.
			menuform->findControlTyped<Graphic>("BRIEFING_IMAGE")
			    ->setImage(fw().data->loadImage("xcom3/tacdata/brief.pcx"));
			briefing = tr(""
				"Search the building for Alien life forms or other hostile forces. Engage the"
				" enemy, but where possible stun Aliens using a Stun Grapple, Stun Grenade, "
				"or Psionic power. Live Aliens are essential for our research. If all hostile"
				" units are eliminated or stunned then we can recover any equipment and Alien"
				" artifacts. A Bio-Transport module must be at the investigation site to "
				"enable the recovery of unconscious or dead Aliens. Be careful to avoid "
				"endangering any civilians and remember that the organization which owns the "
			    "building will not be pleased if there is extensive damage to the structure.");
		}
		else
		{
			if (building->owner != state->getAliens())
			{
				// Raid against a human organisation.
				menuform->findControlTyped<Graphic>("BRIEFING_IMAGE")
				    ->setImage(fw().data->loadImage("xcom3/tacdata/brief2.pcx"));
				briefing = tr(""
					"Raiding forces must eliminate all other forces, whether they are raiders or "
					"defenders. Raiders are deployed on the edge of the combat zone. All "
					"defending forces are allied with each other and must repel any raiders. "
					"Defenders are deployed in the center of the combat zone");
			}
			else
			{
				// Raid against an alien building: unique briefing for each building.
				int briefingID = alienFunctionMap.at(building->function->name);
				menuform->findControlTyped<Graphic>("BRIEFING_IMAGE")
				    ->setImage(
				        fw().data->loadImage(format("xcom3/tacdata/alienm%d.pcx", briefingID)));
				switch (briefingID)
				{
					case 1:
						briefing = tr(""
						    "The Megapods are the means by which the Aliens create new structures. "
						    "The "
						    "small Egg-like objects are eventually replanted and grow into massive "
						    "organic structures. Our discoveries are shocking; this building is "
						    "practically overflowing with Pods. Our Scientists fear that the "
						    "Aliens are "
						    "planning a massive expansion and who knows how we could stop them "
						    "then. All "
						    "Megapods must be destroyed thus preventing the Aliens building any "
						    "new "
						    "structures.");
						break;
					case 2:
						briefing = tr(""
						              "The Control Chamber houses giant organic brains which "
						              "control the operation "
						              "of Alien entities within the Alien dimension. The "
						              "destruction of these "
						              "organic brains will make any remaining Aliens more "
						              "desperate, as their "
						              "ultimate defeat becomes an imminent reality.");
						break;
					case 3:
						briefing = tr(""
							"The destruction of the Food Chambers leads us to the Alien Farm. This "
							"contains a number of strange white blocks. Our Scientists believe that these"
							" curious objects influence the atmospheric conditions of the Alien "
							"Dimension, although it has been impossible to prove this. Whatever the "
							"purpose of these blocks, their destruction can only hinder the Alien cause. "
							"Research indicates that the blocks are located in multiple locations, "
							"although only this site has been photographed. Destroy all of the blocks to "
							"disable the building.");
						break;
					case 4:
						briefing = tr(""
						    "The Food Chamber contains the Aliens' food source in the form of "
						    "plants. "
						    "These plants require organic heat and light sources to prevent them "
						    "from "
						    "decaying. The Mutant alliance also informs us that a number of "
						    "Sectoids are "
						    "held captive within the Food Chamber. Our old enemy has apparently "
						    "become an"
						    " Alien delicacy. Whilst the rescue of any Sectoids will guarantee an "
						    "Alliance with the Mutants, the primary objective is to destroy the "
						    "Alien "
						    "heat and light sources as indicated here.");
						break;
					case 5:
						briefing = tr(""
						    "The destruction of the Dimension Gates is our ultimate goal. From "
						    "evidence "
						    "collected at previous Alien buildings, we have managed to composite "
						    "this "
						    "picture of our objectives. The Alien Generators must be destroyed, "
						    "simultaneously disabling the protective laser web. Destroy all of the "
						    "generators to disable the building. Upon disabling the building it is "
						    "imperative that all Agents evacuate as a matter of urgency. Our "
						    "forces must "
						    "return to the Earth dimension before the final Dimension Gate closes "
						    "forever.Victory is in our sights - Good Luck!");
						break;
					case 6:
						briefing = tr(""
							"The Incubator Chamber contains Alien Eggs. These Eggs are "
							"held at an exact "
							"temperature inside Incubators providing an environment for "
							"the Eggs to hatch"
							" at an optimum rate. Research reveals that a number of "
							"Incubators exist, "
							"they must all be destroyed.");
						break;
					case 7:
						briefing = tr(""
						    "The Maintenance Factory appears to contain a number of sacred Alien "
						    "structures. We believe that the Alien Dimension is fueled by the "
						    "structures "
						    "pictured here. These heart units must be destroyed in order to weaken "
						    "the "
						    "remaining structures. Our success here will indeed be a severe blow "
						    "to the "
						    "Aliens as exactly half of the Alien Dimension will lie in "
						    "ruins.");
						break;
					case 8:
						briefing = tr(""
						    "The Organic Factory provides a construction center for "
						    "Alien UFOs. In their "
						    "initial stage of development the UFOs resemble small "
						    "mushroom-like objects. "
						    "These objects increase in size until they reach the "
						    "colossal sizes of the "
						    "UFOs we have encountered. When fully grown the UFOs detach "
						    "themselves from "
						    "their stem and become fully functional Alien attack "
						    "vessels. All embryonic "
						    "UFOs must be destroyed.");
						break;
					case 9:
						briefing = tr(""
							"The Alien Dimension contains many structures linked by an "
							"irregular snaking "
							"chain. The Sleeping Chamber lies at the start of the chain "
							"and allows the "
							"Aliens to rejuvenate nocturnally. The Aliens regularly "
							"connect themselves to"
							" sleeping units, which appears to be a necessary operation "
							"in order for them"
							" to stay alive. Explore the area with caution and destroy "
							"all sleeping units"
							" as pictured here. After disabling the building, all Agents "
							"must exit the as"
							" soon as possible.");
						break;
					case 10:
						briefing = tr(""
							"The Spawning Chamber appears to be a very special structure. Very few Aliens"
							" enter this structure although vast numbers of Aliens regularly leave the "
							"building. Our research indicates that this building is the lair for some "
							"kind of Alien queen. We believe this Alien to be the sole producer of Alien "
							"Eggs from which all Aliens hatch. Whilst the primary objective is the "
							"destruction of the Queen and all Alien Eggs, the live capture of the Alien "
							"Queen would be a vicious insult to the Aliens.");
						break;
				}
			}
		}
	}
	menuform->findControlTyped<Label>("TEXT_BRIEFING")->setText(briefing);
	/*menuform->findControlTyped<Label>("TEXT_BRIEFING")
	    ->setText("You must lorem ipisum etc. (Here be briefing text)");
*/
	menuform->findControlTyped<GraphicButton>("BUTTON_REAL_TIME")->setVisible(false);
	menuform->findControlTyped<GraphicButton>("BUTTON_TURN_BASED")->setVisible(false);

	menuform->findControlTyped<GraphicButton>("BUTTON_REAL_TIME")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_battle->setMode(Battle::Mode::RealTime);
		    fw().stageQueueCommand(
		        {StageCmd::Command::REPLACEALL, mksp<BattlePreStart>(this->state)});
	    });

	menuform->findControlTyped<GraphicButton>("BUTTON_TURN_BASED")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_battle->setMode(Battle::Mode::TurnBased);
		    fw().stageQueueCommand(
		        {StageCmd::Command::REPLACEALL, mksp<BattlePreStart>(this->state)});
	    });
}

void BattleBriefing::begin() {}

void BattleBriefing::pause() {}

void BattleBriefing::resume() {}

void BattleBriefing::finish() {}

void BattleBriefing::eventOccurred(Event *e)
{
	menuform->eventOccured(e);
	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_RETURN || e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_REAL_TIME")->click();
			return;
		}
		if (e->keyboard().KeyCode == SDLK_SPACE)
		{
			menuform->findControl("BUTTON_TURN_BASED")->click();
			return;
		}
	}
}

void BattleBriefing::update()
{
	menuform->update();

	auto status = this->loading_task.wait_for(std::chrono::seconds(0));
	switch (status)
	{
		case std::future_status::ready:
		{
			menuform->findControlTyped<GraphicButton>("BUTTON_REAL_TIME")->setVisible(true);
			menuform->findControlTyped<GraphicButton>("BUTTON_TURN_BASED")->setVisible(true);
		}
			return;
		default:
			// Not yet finished
			return;
	}
}

void BattleBriefing::render()
{
	menuform->render();
	// FIXME: Draw "loading" in the bottom ?
}

bool BattleBriefing::isTransition() { return false; }

}; // namespace OpenApoc
