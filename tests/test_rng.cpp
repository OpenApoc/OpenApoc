#include "framework/logger.h"
#include "library/xorshift.h"

using namespace OpenApoc;

int main(int argc, char **argv)
{
	xorshift_128_plus<uint32_t> rng{};

	uint32_t r1 = rng();
	uint32_t r2 = rng();

	uint32_t expected_r1 = 0x2f919517;
	uint32_t expected_r2 = 0x2766980d;
	uint32_t expected_r3 = 0x68192db7;

	if (r1 != expected_r1)
	{
		LogError("unexpected r1 0x%08x, expected 0x%08x", r1, expected_r1);
		return EXIT_FAILURE;
	}

	if (r2 != expected_r2)
	{
		LogError("unexpected r2 0x%08x, expected 0x%08x", r2, expected_r2);
		return EXIT_FAILURE;
	}

	// Save the state to another rng and check that result matches

	uint32_t s[2];
	xorshift_128_plus<uint32_t> rng2{};

	rng.get_state(s);
	rng2.set_state(s);

	uint32_t r3 = rng2();
	if (r3 != expected_r3)
	{
		LogError("unexpected r3 0x%08x, expected 0x%08x", r3, expected_r3);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
