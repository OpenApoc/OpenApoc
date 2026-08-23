#include "framework/configfile.h"
#include "framework/logger.h"
#include "library/strings.h"

using namespace OpenApoc;

static bool checkCurrencyFormatting(const UString &input, const UString &expected)
{
	const auto actual = Strings::formatTextAsCurrency(input);
	if (actual != expected)
	{
		LogError("formatTextAsCurrency(\"{0}\") returned \"{1}\", expected \"{2}\"", input, actual,
		         expected);
		return false;
	}
	return true;
}

int main(int argc, char **argv)
{
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	const std::vector<std::pair<UString, UString>> examples = {
	    {"", ""},
	    {"0", "0"},
	    {"123", "123"},
	    {"1234", "1,234"},
	    {"1234567", "1,234,567"},
	    {"-1234567", "-1,234,567"},
	    {"1,234", "1,234"},
	    {"1,2,3,4", "1,234"},
	    {"1234.56", "1,234.56"},
	    {"-1234.56", "-1,234.56"},
	    {"1234.", "1,234."},
	    {"not money", "not money"},
	    {"$1234", "$1234"},
	    {"12-34", "12-34"},
	    {"1234.56.78", "1234.56.78"},
	};

	for (const auto &example : examples)
	{
		if (!checkCurrencyFormatting(example.first, example.second))
		{
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
