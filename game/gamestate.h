#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class City;

class GameState {
public:
	std::unique_ptr<City> city;
	void clear();
};

}; //namespace OpenApoc
