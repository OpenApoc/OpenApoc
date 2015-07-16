
#include "ufopaediaentry.h"

namespace OpenApoc {

class UfopaediaCategory
{
	public:
		UString ID;
		UString Title;
		UString BodyInformation;
		UString BackgroundImageFilename;
		std::vector<std::shared_ptr<UfopaediaEntry>> Entries;

		UfopaediaCategory(tinyxml2::XMLElement* Element);
		~UfopaediaCategory();
};
}; //namespace OpenApoc
