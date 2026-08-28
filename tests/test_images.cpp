#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/logger.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>

using namespace OpenApoc;

namespace
{

// Minimal SHA-256 (FIPS 180-4), self-contained so the expected digests are stable across
// platforms and third-party library versions.
class Sha256
{
  public:
	Sha256()
	    : state{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
	            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19}
	{
	}

	void update(const uint8_t *data, size_t len)
	{
		total += len;
		while (len > 0)
		{
			const size_t space = block.size() - blockLen;
			const size_t take = len < space ? len : space;
			std::memcpy(block.data() + blockLen, data, take);
			blockLen += take;
			data += take;
			len -= take;
			if (blockLen == block.size())
			{
				compress();
				blockLen = 0;
			}
		}
	}

	UString hexDigest()
	{
		const uint64_t bitLen = total * 8;
		const uint8_t pad = 0x80;
		update(&pad, 1);
		const uint8_t zero = 0x00;
		while (blockLen != 56)
		{
			update(&zero, 1);
		}
		for (int i = 7; i >= 0; i--)
		{
			const uint8_t b = static_cast<uint8_t>(bitLen >> (i * 8));
			update(&b, 1);
		}
		UString out;
		for (const uint32_t word : state)
		{
			for (int i = 28; i >= 0; i -= 4)
			{
				out += "0123456789abcdef"[(word >> i) & 0xf];
			}
		}
		return out;
	}

  private:
	static uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

	void compress()
	{
		static const uint32_t k[64] = {
		    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
		    0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
		    0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
		    0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
		    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
		    0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
		    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
		    0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
		    0xc67178f2};
		uint32_t w[64];
		for (int i = 0; i < 16; i++)
		{
			w[i] = (uint32_t(block[i * 4]) << 24) | (uint32_t(block[i * 4 + 1]) << 16) |
			       (uint32_t(block[i * 4 + 2]) << 8) | uint32_t(block[i * 4 + 3]);
		}
		for (int i = 16; i < 64; i++)
		{
			const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
			const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
			w[i] = w[i - 16] + s0 + w[i - 7] + s1;
		}
		uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
		uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
		for (int i = 0; i < 64; i++)
		{
			const uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
			const uint32_t ch = (e & f) ^ (~e & g);
			const uint32_t t1 = h + s1 + ch + k[i] + w[i];
			const uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
			const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
			const uint32_t t2 = s0 + maj;
			h = g;
			g = f;
			f = e;
			e = d + t1;
			d = c;
			c = b;
			b = a;
			a = t1 + t2;
		}
		state[0] += a;
		state[1] += b;
		state[2] += c;
		state[3] += d;
		state[4] += e;
		state[5] += f;
		state[6] += g;
		state[7] += h;
	}

	std::array<uint32_t, 8> state;
	std::array<uint8_t, 64> block{};
	size_t blockLen = 0;
	uint64_t total = 0;
};

// Digest of the decoded image: "WxH:" in ASCII, then row-major RGBA bytes. Keeping the
// dimensions in text form makes the digest independent of host endianness.
UString imageHash(sp<RGBImage> img)
{
	Sha256 sha;
	const auto header = format("{0}x{1}:", img->size.x, img->size.y);
	sha.update(reinterpret_cast<const uint8_t *>(header.c_str()), header.length());
	RGBImageLock lock(img, ImageLockUse::Read);
	for (unsigned int y = 0; y < img->size.y; y++)
	{
		for (unsigned int x = 0; x < img->size.x; x++)
		{
			const auto c = lock.get({x, y});
			const uint8_t px[4] = {c.r, c.g, c.b, c.a};
			sha.update(px, sizeof(px));
		}
	}
	return sha.hexDigest();
}

bool testImage(const UString &imageName, const UString &expectedHash)
{
	auto img = fw().data->loadImage(imageName);
	if (!img)
	{
		LogWarning("Failed to load image");
		return false;
	}

	auto rgbImg = std::dynamic_pointer_cast<RGBImage>(img);
	if (!rgbImg)
	{
		LogWarning("Image not RGBImage");
		return false;
	}

	const auto hash = imageHash(rgbImg);
	if (hash != expectedHash)
	{
		LogWarning("Hash mismatch: computed {0}, expected {1}", hash, expectedHash);
		std::cout << "HASH " << imageName << " " << hash << "\n";
		return false;
	}
	return true;
}

} // namespace

int main(int argc, char **argv)
{
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}
	// Expected SHA-256 digests of the decoded RGBA output ("WxH:" prefix plus row-major RGBA
	// bytes). To regenerate after an intentional decoder change, run this test and copy the
	// digests it prints on mismatch.
	std::map<UString, UString> testImages = {
	    // PCX files:
	    {"xcom3/ufodata/titles.pcx",
	     "743b409cda0273d6e2fbc919fab0fc7a81d274938ff6f6701442b93b8f7e0abe"},
	    // RAW files:
	    {"RAW:xcom3/ufodata/isobord1.dat:640:128:xcom3/ufodata/pal_01.dat",
	     "87c6c94dd741816f8676f774d5762ff4df526f772c6559c1e69b9e2514e9bf55"},
	    // PCK files:
	    {"PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:30:xcom3/ufodata/base.pcx",
	     "37d9310915c859ad8bdcffdaae8a5ff70db3b51ebba320faed16153b402dffad"},
	    {"PCK:xcom3/ufodata/city.pck:xcom3/ufodata/city.tab:956:xcom3/ufodata/pal_01.dat",
	     "efda230ebd94b917807952f1fe250b7f53f3298fe6f9d0d7b8a54755c37fd70a"},
	    {"PCK:xcom3/tacdata/unit/xcom1a.pck:xcom3/tacdata/unit/xcom1a.tab:30:xcom3/tacdata/"
	     "tactical.pal",
	     "04c1e99dc7c76e7024928e95ad709fac2011c12b4581bd724dd715c2a70c9fc3"},
	    {"PCK:xcom3/tacdata/unit/xcom1a.pck:xcom3/tacdata/unit/xcom1a.tab:240:xcom3/tacdata/"
	     "tactical.pal",
	     "4d7079ff66a3014c80bda5f4422afd384fce1c663644d81af74d5cd1e40b447d"},
	    {"PCK:xcom3/tacdata/unit/xcom1a.pck:xcom3/tacdata/unit/xcom1a.tab:134:xcom3/tacdata/"
	     "tactical.pal",
	     "6a43de0f9dbe0464748836fd20611ee35bb982a9341500942ca2c41ffd42fcf4"},
	    // PCKSTRAT files:
	    {"PCKSTRAT:xcom3/ufodata/stratmap.pck:xcom3/ufodata/stratmap.tab:32:xcom3/ufodata/"
	     "pal_01.dat",
	     "6193f4092bcb45be629588ce1258d2a0ff26e327d74f1bfc9c99a666b2d13b7f"},
	    // SHADOW files:
	    {"PCKSHADOW:xcom3/ufodata/shadow.pck:xcom3/ufodata/shadow.tab:5:xcom3/ufodata/pal_01.dat",
	     "fb987a9d0f1421e7b0edf1469725cd7ef6642bf1abde7dd489dd8779294f89db"},
	    // LOFTEMPS files:
	    {"LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/ufodata/loftemps.tab:113",
	     "f5c0e2ae9e37e56cfeef8aafa46f04b2f0b4c24f21f71eb3e290e31ac3825b5e"},
	    {"LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/ufodata/loftemps.tab:150",
	     "7572f27e8c8b10ed3cbb9c5a1302048b3674cf123556a47b0ecb64a41bab1a94"},
	    {"LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/ufodata/loftemps.tab:151",
	     "26c9d9409859c8e22a96ff9eb8e61d85415e708eb536950acb131e4fda93cf87"},
	};
	Framework fw("OpenApoc", false);

	bool failed = false;
	for (auto &imagePair : testImages)
	{
		if (!testImage(imagePair.first, imagePair.second))
		{
			LogError("Image \"{0}\" didn't match expected hash \"{1}\"", imagePair.first,
			         imagePair.second);
			failed = true;
		}
	}

	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
