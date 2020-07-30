#include "framework/apocresources/pck.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/logger.h"
#include "library/sp.h"
#include <istream>
#include <vector>

namespace OpenApoc
{

static const unsigned int IMAGE_STRIDE = 640;

#pragma pack(push, 1)
struct PckHeader
{
	uint16_t compressionMode;
	uint16_t unknown;
	uint16_t leftClip;
	uint16_t rightClip;
	uint16_t topClip;
	uint16_t bottomClip;
};
#pragma pack(pop)
static_assert(sizeof(struct PckHeader) == 12, "PckHeader not 12 bytes");

#pragma pack(push, 1)
struct PckRLE1Header
{
	uint32_t pixelSkip;
	uint8_t column;
	uint16_t pixelCount;
	uint8_t padding;
};
#pragma pack(pop)
static_assert(sizeof(struct PckRLE1Header) == 8, "RLE1Header not 8 bytes");

static sp<PaletteImage> readPckCompression1(std::istream &input, Vec2<unsigned> size)
{
	auto img = mksp<PaletteImage>(size);

	struct PckRLE1Header header;

	input.read(reinterpret_cast<char *>(&header), sizeof(header));

	PaletteImageLock l(img, ImageLockUse::Write);

	while (input && header.pixelSkip != 0xffffffff)
	{
		unsigned int col = header.pixelSkip % IMAGE_STRIDE;

		if (col != header.column)
		{
			LogWarning("Header column %u doesn't match skip column %u (%u %% %u)",
			           (unsigned)header.column, col, (unsigned)header.pixelSkip, IMAGE_STRIDE);
			return nullptr;
		}

		for (unsigned int i = 0; i < header.pixelCount; i++)
		{
			unsigned int x = (i + header.pixelSkip) % IMAGE_STRIDE;
			unsigned int y = (i + header.pixelSkip) / IMAGE_STRIDE;
			uint8_t idx = 0;
			input.read(reinterpret_cast<char *>(&idx), sizeof(idx));
			if (!input)
			{
				LogWarning("Unexpected EOF reading Pck RLE data");
				return nullptr;
			}
			if (x < size.x && y < size.y)
				l.set({x, y}, idx);
		}
		input.read(reinterpret_cast<char *>(&header), sizeof(header));
	}
	return img;
}

#pragma pack(push, 1)
struct PckBlkHeader
{
	uint8_t rowRecords;
	uint16_t unknown;
	uint8_t row;
};
#pragma pack(pop)
static_assert(sizeof(struct PckBlkHeader) == 4, "BlkHeader not 4 bytes");

#pragma pack(push, 1)
struct PckBlkSubHeader
{
	uint8_t pixelSkip;
	uint8_t pixelCount;
	// MSVC doesn't like aligning bit fields, this is really a 24bit 'little endian' offset
	uint8_t blkOffset[3];
};
#pragma pack(pop)
static_assert(sizeof(struct PckBlkSubHeader) == 5, "BlkSubHeader not 5 bytes");

static sp<PaletteImage> readPckCompression3(std::istream &input, Vec2<unsigned> size)
{
	static size_t blkSize;
	static up<char[]> blkData;

	if (!blkData)
	{
		auto blkFile = fw().data->fs.open("xcom3/tacdata/xcom.blk");
		if (!blkFile)
		{
			LogWarning("Failed to open xcom.blk");
			return nullptr;
		}
		blkSize = blkFile.size();
		blkData = blkFile.readAll();
		LogInfo("Loaded %zu bytes of xcom.blk", blkSize);
	}

	auto img = mksp<PaletteImage>(size);

	PaletteImageLock l(img, ImageLockUse::Write);

	struct PckBlkHeader header;

	input.read(reinterpret_cast<char *>(&header), sizeof(header));

	while (input && header.rowRecords != 0xff && header.unknown != 0xffff && header.row != 0xff)
	{
		unsigned col = 0;
		unsigned row = header.row;
		for (unsigned record = 0; record < header.rowRecords; record++)
		{
			struct PckBlkSubHeader subHeader;
			input.read(reinterpret_cast<char *>(&subHeader), sizeof(subHeader));
			if (!input)
			{
				LogWarning("Unexpected EOF reading row header");
				return nullptr;
			}
			unsigned blkOffset = 0;
			blkOffset =
			    subHeader.blkOffset[0] | subHeader.blkOffset[1] << 8 | subHeader.blkOffset[2] << 16;
			col += subHeader.pixelSkip;
			for (unsigned i = 0; i < subHeader.pixelCount; i++)
			{
				if (blkOffset >= blkSize)
				{
					LogWarning("BLKOffset %u too large for xcom.blk size", blkOffset);
				}
				else
				{
					if (row < size.y && col < size.x)
					{
						l.set({col, row}, blkData[blkOffset]);
					}
					else
					{
						LogWarning("{%d,%d} out of bounds", col, row);
					}
				}
				blkOffset++;
				col++;
			}
		}

		input.read(reinterpret_cast<char *>(&header), sizeof(header));
	}
	return img;
}

static unsigned int guessTabMultiplier(IFile &pckFile, IFile &tabFile)
{
	// This tries to guess if the tab file contains (offset) or (offset/4) based on the last entry,
	// if multiplying it by 4 is greater than the pck file size it's (offset), otherwise (offset/4)
	auto pckSize = pckFile.size();
	auto tabSize = tabFile.size();
	if (tabSize < 4)
	{
		LogWarning("Tab size %zu too small for a single entry?", tabSize);
		return 0;
	}

	// Store the tab offset so we can restore the file state
	auto tabOffset = tabFile.tellg();
	tabFile.seekg(tabSize - 4);
	uint32_t lastOffset;
	tabFile.read(reinterpret_cast<char *>(&lastOffset), sizeof(lastOffset));
	if (!tabFile)
	{
		LogWarning("Failed to read last tab offset");
		return 0;
	}
	tabFile.seekg(tabOffset);
	if (lastOffset * 4 >= pckSize)
	{
		return 1;
	}
	else
	{
		return 4;
	}
}

sp<ImageSet> PCKLoader::load(Data &d, UString PckFilename, UString TabFilename)
{
	auto imageSet = mksp<ImageSet>();
	auto pck = d.fs.open(PckFilename);
	if (!pck)
	{
		LogError("Failed to open PCK file \"%s\"", PckFilename);
		return nullptr;
	}
	auto tab = d.fs.open(TabFilename);
	if (!tab)
	{
		LogError("Failed to open TAB file \"%s\"", TabFilename);
		return nullptr;
	}

	auto tabMultiplier = guessTabMultiplier(pck, tab);
	if (tabMultiplier == 0)
	{
		LogWarning("Failed to guess tab file type for \"%s\"", TabFilename);
		return nullptr;
	}

	LogInfo("Reading \"%s\" with tab multiplier %u", TabFilename, tabMultiplier);

	unsigned int endIdx = (tab.size() / 4);

	imageSet->images.resize(endIdx);
	imageSet->maxSize = {0, 0};

	for (unsigned i = 0; i < endIdx; i++)
	{
		uint32_t pckOffset = 0;
		tab.seekg(i * 4, std::ios_base::beg);
		tab.read(reinterpret_cast<char *>(&pckOffset), sizeof(pckOffset));
		if (!tab)
		{
			LogWarning("Reached EOF reading tab index %u", i);
			return nullptr;
		}
		pckOffset *= tabMultiplier;
		struct PckHeader header;
		pck.seekg(pckOffset, std::ios_base::beg);
		pck.read(reinterpret_cast<char *>(&header), sizeof(header));
		if (!pck)
		{
			LogInfo("Reached EOF reading PCK header at tab index %u", i);
			break;
		}
		sp<PaletteImage> img;

		switch (header.compressionMode)
		{
			case 0:
				// 0 appears to mean a missing image?
				img = mksp<PaletteImage>(Vec2<unsigned>{1, 1});
				break;
			case 1:
				img = readPckCompression1(pck, {header.rightClip, header.bottomClip});
				break;
			case 3:
				img = readPckCompression3(pck, {header.rightClip, header.bottomClip});
				break;
			default:
				LogWarning("Unknown compression mode %u", (unsigned)header.compressionMode);
				break;
		}
		if (!img)
		{
			LogInfo("No image at PCK index %u", i);
			continue;
		}
		img->calculateBounds();
		img->owningSet = imageSet;
		imageSet->images[i] = img;

		if (img->size.x > imageSet->maxSize.x)
			imageSet->maxSize.x = img->size.x;
		if (img->size.y > imageSet->maxSize.y)
			imageSet->maxSize.y = img->size.y;
	}
	return imageSet;
}

struct StratHeader
{
	uint16_t pixel_skip; // if 0xffff end-of-sprite
	uint16_t count;      // The number of indices following this
};

static_assert(sizeof(struct StratHeader) == 4, "Invalid strat_header size");

static sp<PaletteImage> loadStrategy(IFile &file)
{
	auto img = mksp<PaletteImage>(Vec2<int>{8, 8}); // All strategy map tiles are 8x8
	unsigned int offset = 0;

	struct StratHeader header;
	file.read(reinterpret_cast<char *>(&header), sizeof(header));
	PaletteImageLock region(img);
	while (file && header.pixel_skip != 0xffff)
	{
		unsigned int count = header.count;
		offset = header.pixel_skip;
		while (file && count--)
		{
			uint8_t idx = 0;
#define STRIDE 640
			unsigned int x = offset % STRIDE;
			unsigned int y = offset / STRIDE;
#undef STRIDE

			file.read(reinterpret_cast<char *>(&idx), 1);

			if (x >= 8 || y >= 8)
			{
				LogInfo("Writing to {%d,%d} in 8x8 stratmap image", x, y);
			}
			else
			{
				region.set(Vec2<int>{x, y}, idx);
			}

			offset++;
		}
		file.read(reinterpret_cast<char *>(&header), sizeof(header));
	}
	return img;
}

sp<ImageSet> PCKLoader::loadStrat(Data &data, UString PckFilename, UString TabFilename)
{
	auto imageSet = mksp<ImageSet>();
	auto tabFile = data.fs.open(TabFilename);
	if (!tabFile)
	{
		LogWarning("Failed to open tab \"%s\"", TabFilename);
		return nullptr;
	}
	auto pckFile = data.fs.open(PckFilename);
	if (!pckFile)
	{
		LogWarning("Failed to open tab \"%s\"", TabFilename);
		return nullptr;
	}

	uint32_t offset = 0;
	unsigned idx = 0;
	while (tabFile.read(reinterpret_cast<char *>(&offset), sizeof(offset)))
	{
		pckFile.seekg(offset, std::ios::beg);
		if (!pckFile)
		{
			LogError("Failed to seek to offset %u", offset);
			return nullptr;
		}
		auto img = loadStrategy(pckFile);
		if (!img)
		{
			LogError("Failed to load image");
			return nullptr;
		}
		if (img->size != Vec2<unsigned int>{8, 8})
		{
			LogError("Invalid size of {%d,%d} in stratmap image", img->size.x, img->size.y);
			return nullptr;
		}
		imageSet->images.push_back(img);
		img->owningSet = imageSet;
		img->calculateBounds();
		img->indexInSet = idx++;
	}

	imageSet->maxSize = {8, 8};

	LogInfo("Loaded %u images", static_cast<unsigned>(imageSet->images.size()));

	return imageSet;
}

struct ShadowHeader
{
	uint8_t h1;
	uint8_t h2;
	uint16_t unknown1;
	uint16_t width;
	uint16_t height;
};

static_assert(sizeof(struct ShadowHeader) == 8, "Invalid shadow_header size");

static const std::vector<std::vector<int>> ditherLut = {
    {0, 0, 0, 0}, {1, 0, 1, 0}, {0, 1, 0, 1}, {1, 0, 0, 0},
    {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1},
};

static sp<PaletteImage> loadShadowImage(IFile &file, uint8_t shadedIdx)
{
	struct ShadowHeader header;
	file.read(reinterpret_cast<char *>(&header), sizeof(header));
	if (!file)
	{
		LogError("Unexpected EOF reading shadow PCK header\n");
		return nullptr;
	}
	auto img = mksp<PaletteImage>(Vec2<int>{header.width, header.height});
	PaletteImageLock region(img);

	uint8_t b = 0;
	file.read(reinterpret_cast<char *>(&b), 1);
	int pos = 0;
	while (b != 0xff)
	{
		uint8_t count = b;
		file.read(reinterpret_cast<char *>(&b), 1);
		if (!file)
		{
			LogError("Unexpected EOF reading shadow data\n");
			return nullptr;
		}
		uint8_t idx = b;

		if (idx == 0)
			pos += count * 4;
		else
		{
			LogAssert(idx < 7);

			while (count--)
			{
				for (int i = 0; i < 4; i++)
				{
					const int STRIDE = 640;
					int x = pos % STRIDE;
					int y = pos / STRIDE;
					if (x < header.width && y < header.height)
					{
						if (ditherLut[idx][i])
							region.set({x, y}, shadedIdx);
						else
							region.set({x, y}, 0);
					}
					pos++;
				}
			}
		}
		file.read(reinterpret_cast<char *>(&b), 1);
		if (!file)
		{
			LogError("Unexpected EOF reading shadow data\n");
			return nullptr;
		}
	}
	return img;
}

sp<ImageSet> PCKLoader::loadShadow(Data &data, UString PckFilename, UString TabFilename,
                                   unsigned shadedIdx)
{
	auto imageSet = mksp<ImageSet>();
	auto tabFile = data.fs.open(TabFilename);
	if (!tabFile)
	{
		LogWarning("Failed to open tab \"%s\"", TabFilename);
		return nullptr;
	}
	auto pckFile = data.fs.open(PckFilename);
	if (!pckFile)
	{
		LogWarning("Failed to open tab \"%s\"", TabFilename);
		return nullptr;
	}
	imageSet->maxSize = {0, 0};

	uint32_t offset = 0;
	unsigned idx = 0;
	while (tabFile.read(reinterpret_cast<char *>(&offset), sizeof(offset)))
	{
		// shadow TAB files store the offset directly
		pckFile.seekg(offset, std::ios::beg);
		if (!pckFile)
		{
			LogError("Failed to seek to offset %u", offset);
			return nullptr;
		}
		auto img = loadShadowImage(pckFile, shadedIdx);
		if (!img)
		{
			LogError("Failed to load image");
			return nullptr;
		}
		imageSet->images.push_back(img);
		img->owningSet = imageSet;
		img->calculateBounds();
		img->indexInSet = idx++;
		if (img->size.x > imageSet->maxSize.x)
			imageSet->maxSize.x = img->size.x;
		if (img->size.y > imageSet->maxSize.y)
			imageSet->maxSize.y = img->size.y;
	}

	LogInfo("Loaded %u images", static_cast<unsigned>(imageSet->images.size()));

	return imageSet;
}

}; // namespace OpenApoc
