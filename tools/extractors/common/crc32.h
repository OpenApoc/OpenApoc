#pragma once

/*
 * CRC32 implementation - taken from http://rosettacode.org/wiki/CRC-32#C.2B.2B
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>

// Generates a lookup table for the checksums of all 8-bit values.
static std::array<std::uint_fast32_t, 256> generate_crc_lookup_table()
{

	// This is a function object that calculates the checksum for a value,
	// then increments the value, starting from zero.
	struct ByteChecksum
	{
		const std::uint_fast32_t reversed_polynomial = std::uint_fast32_t{0xEDB88320uL};

		std::uint_fast32_t operator()()
		{
			auto checksum = static_cast<std::uint_fast32_t>(n++);

			for (auto i = 0; i < 8; ++i)
				checksum = (checksum >> 1) ^ ((checksum & 0x1u) ? reversed_polynomial : 0);

			return checksum;
		}

		unsigned n = 0;
	};

	auto table = std::array<std::uint_fast32_t, 256>{};
	std::generate(table.begin(), table.end(), ByteChecksum{});

	return table;
}

// Calculates the CRC for any sequence of values. (You could use type traits and a
// static LogAssert to ensure the values can be converted to 8 bits.)
template <typename InputIterator> std::uint_fast32_t crc(InputIterator first, InputIterator last)
{
	// Generate lookup table only on first use then cache it - this is thread-safe.
	static auto const table = generate_crc_lookup_table();

	// Calculate the checksum - make sure to clip to 32 bits, for systems that don't
	// have a true (fast) 32-bit type.
	return std::uint_fast32_t{0xFFFFFFFFuL} &
	       ~std::accumulate(first, last, ~std::uint_fast32_t{0} & std::uint_fast32_t{0xFFFFFFFFuL},
	                        [](std::uint_fast32_t checksum, std::uint_fast8_t value) {
		                        return table[(checksum ^ value) & 0xFFu] ^ (checksum >> 8);
		                    });
}
