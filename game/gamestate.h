#pragma once
#include "city/city.h"

namespace OpenApoc {

class GameState {
public:
	std::unique_ptr<City> city;
};

}; //namespace OpenApoc
