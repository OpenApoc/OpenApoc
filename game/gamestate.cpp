#include "gamestate.h"
#include "game/city/city.h"

#include "framework/includes.h"

namespace OpenApoc {

void
GameState::clear()
{
	this->city.reset();
}

}; //namespace OpenApoc
