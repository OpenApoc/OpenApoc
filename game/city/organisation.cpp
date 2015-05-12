#include "game/city/organisation.h"

namespace OpenApoc {

std::vector<Organisation> Organisation::defaultOrganisations =
{
	{"X-COM"},
	{"Alien"},
	{"Government"},
	{"Megapol"},
	{"Cult of Sirius"},
	{"Marsec"},
	{"Superdynamics"},
	{"General Metro"},
	{"Cyberweb"},
	{"Transtellar"},
	{"Solmine"},
	{"Sensovision"},
	{"Lifetree"},
	{"Nutrivend"},
	{"Evonet"},
	{"Sanctuary Clinic"},
	{"Nanotech"},
	{"Energen"},
	{"Synthemesh"},
	{"Gravball League"},
	{"Psyke"},
	{"Diablo"},
	{"S.E.L.F."},
	{"Mutant Alliance"},
	{"Extropians"},
	{"Technocrats"},
	{"Civilian"},
};

Organisation::Organisation(std::string name)
	: name(name)
{}

}; //namespace OpenApoc
