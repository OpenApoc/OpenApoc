#include "game/ui/city/commenceInvestigation.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/ui/general/messagebox.h"
#include "library/sp.h"

namespace OpenApoc
{

CommenceInvestigation::CommenceInvestigation() : Stage()
{
	LogWarning("Aliens");
	this->begin();
}

CommenceInvestigation::~CommenceInvestigation() = default;

void CommenceInvestigation::begin()
{
	UString title = tr("Commence investigation");
	UString message =
	    tr("All selected units and crafts have arrived. Proceed with investigation? ");
	fw().stageQueueCommand(
	    {StageCmd::Command::PUSH, mksp<MessageBox>(title, message, MessageBox::ButtonOptions::Ok)});
}

void CommenceInvestigation::pause() {}

void CommenceInvestigation::resume() {}

void CommenceInvestigation::finish() {}

void CommenceInvestigation::eventOccurred(Event *e) {}

void CommenceInvestigation::update() {}

void CommenceInvestigation::render() {}

bool CommenceInvestigation::isTransition() { return false; }
}