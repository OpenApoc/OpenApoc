#include "framework/framework.h"
#include <iostream>

using namespace OpenApoc;

void usage(std::ostream &out, const char *progname)
{
	out << "Usage: " << progname << " (image string) (output png file)\n";
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		usage(std::cerr, argv[0]);
		return EXIT_FAILURE;
	}
	Framework f(UString(argv[0]), {}, false);

	auto img = f.data->load_image(argv[1]);
	if (!img)
	{
		std::cerr << "Failed to load image \"" << argv[1] << "\"\n";
		return EXIT_FAILURE;
	}

	if (!f.data->write_image(argv[2], img))
	{
		std::cerr << "Failed to write output file \"" << argv[2] << "\"\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
