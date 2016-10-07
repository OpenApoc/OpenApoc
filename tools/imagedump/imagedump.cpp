#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/framework.h"
#include <iostream>

using namespace OpenApoc;

int main(int argc, char *argv[])
{
	config().addPositionalArgument("image_string", "Image string to read");
	config().addPositionalArgument("output_file", "Output .png file");

	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	auto outputFile = config().getString("output_file");
	auto inputString = config().getString("image_string");

	if (inputString.empty())
	{
		std::cerr << "Must provide image_string\n";
		config().showHelp();
		return EXIT_FAILURE;
	}

	if (outputFile.empty())
	{
		std::cerr << "Must provide output_file\n";
		config().showHelp();
		return EXIT_FAILURE;
	}

	Framework f(UString(argv[0]), false);

	auto img = f.data->loadImage(inputString);
	if (!img)
	{
		std::cerr << "Failed to load image \"" << inputString << "\"\n";
		return EXIT_FAILURE;
	}

	if (!f.data->writeImage(outputFile, img))
	{
		std::cerr << "Failed to write output file \"" << outputFile << "\"\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
