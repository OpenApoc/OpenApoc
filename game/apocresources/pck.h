
#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Data;
class ImageSet;

class PCKLoader {
public:
	static std::shared_ptr<ImageSet> load(Data &data, const std::string PckFilename, const std::string TabFilename);
};

}; //namespace OpenApoc
