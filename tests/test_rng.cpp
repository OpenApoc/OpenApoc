#include "framework/configfile.h"
#include "framework/logger.h"
#include "library/xorshift.h"

using namespace OpenApoc;

int main(int argc, char **argv)
{
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}
	Xorshift128Plus<uint64_t> rng{};

	uint64_t r1 = rng();
	uint64_t r2 = rng();

	uint64_t expected_r1 = 0x03aacfee1f751183;
	uint64_t expected_r2 = 0xcb8aa3521c8fc259;
	uint64_t expected_r3 = 0xdd420b258a17fa82;

	if (r1 != expected_r1)
	{
		LogError("unexpected r1 0x%016x, expected 0x%016x", r1, expected_r1);
		return EXIT_FAILURE;
	}

	if (r2 != expected_r2)
	{
		LogError("unexpected r2 0x%016x, expected 0x%016x", r2, expected_r2);
		return EXIT_FAILURE;
	}

	// Reseed and check it matches
	rng.seed(0);
	r1 = rng();
	r2 = rng();

	if (r1 != expected_r1)
	{
		LogError("unexpected post-reseed r1 0x%016x, expected 0x%016x", r1, expected_r1);
		return EXIT_FAILURE;
	}

	if (r2 != expected_r2)
	{
		LogError("unexpected post-reseed r2 0x%016x, expected 0x%016x", r2, expected_r2);
		return EXIT_FAILURE;
	}

	// Save the state to another rng and check that result matches

	uint64_t s[2];
	Xorshift128Plus<uint64_t> rng2{};

	rng.getState(s);
	rng2.setState(s);

	uint64_t r3 = rng2();
	if (r3 != expected_r3)
	{
		LogError("unexpected r3 0x%016x, expected 0x%016x", r3, expected_r3);
		return EXIT_FAILURE;
	}

	constexpr int num_test_buckets = 4;
	constexpr int num_test_iterations = 500000;

	unsigned buckets[num_test_buckets];
	for (int i = 0; i < num_test_buckets; i++)
	{
		buckets[i] = 0;
	}

	for (int i = 0; i < num_test_iterations; i++)
	{
		auto value = randBoundsExclusive(rng, 0, num_test_buckets);
		buckets[value]++;
	}

	LogWarning("RNG buckets:");
	for (int i = 0; i < num_test_buckets; i++)
	{
		LogWarning("%d:\t%u", i, buckets[i]);
	}

	return EXIT_SUCCESS;
}
