#include "framework/apocresources/pck.h"
#include "framework/data.h"
#include "framework/image.h"
#include "framework/logger.h"
#include "framework/trace.h"
#include "library/sp.h"

namespace OpenApoc
{

namespace
{

typedef struct PCKCompression1ImageHeader
{
	uint8_t Reserved1;
	uint8_t Reserved2;
	uint16_t LeftMostPixel;
	uint16_t RightMostPixel;
	uint16_t TopMostPixel;
	uint16_t BottomMostPixel;
} PCKImageHeader;

typedef struct PCKCompression1RowHeader
{
	// int16_t SkipPixels; -- Read seperately to get eof record
	uint8_t ColumnToStartAt;
	uint8_t PixelsInRow;
	uint8_t BytesInRow;
	uint8_t PaddingInRow;
} PCKCompression1Header;

class PCK
{

  private:
	void processFile(Data &d, UString PckFilename, UString TabFilename, int Index);
	void loadVersion1Format(IFile &pck, IFile &tab, int Index);
	void loadVersion2Format(IFile &pck, IFile &tab, int Index);

  public:
	PCK(Data &d, UString PckFilename, UString TabFilename);
	~PCK();

	std::vector<sp<PaletteImage>> images;
};
PCK::PCK(Data &d, UString PckFilename, UString TabFilename)
{
	processFile(d, PckFilename, TabFilename, -1);
}

PCK::~PCK() = default;

void PCK::processFile(Data &d, UString PckFilename, UString TabFilename, int Index)
{
	auto pck = d.fs.open(PckFilename);
	if (!pck)
	{
		LogError("Failed to open PCK file \"%s\"", PckFilename.cStr());
		return;
	}
	auto tab = d.fs.open(TabFilename);
	if (!tab)
	{
		LogError("Failed to open TAB file \"%s\"", TabFilename.cStr());
		return;
	}

	uint16_t version;
	if (!pck.readule16(version))
	{
		LogError("Failed to read version from \"%s\"", PckFilename.cStr());
		return;
	}
	pck.seekg(0, std::ios::beg);
	switch (version)
	{
		case 0:
			loadVersion1Format(pck, tab, Index);
			break;
		case 1:
			loadVersion2Format(pck, tab, Index);
			break;
	}
}

// FIXME: Replace Memory with vector<char> or something? Or jhust change the loader to write
// directly into the Image?
class Memory
{
  private:
	std::vector<char> data;

  public:
	Memory(size_t InitialSize = 0) : data(InitialSize) {}

	size_t getSize() { return data.size(); }

	void resize(size_t length) { data.resize(length); }

	void appendData(void *data, size_t length)
	{
		size_t startSize = this->getSize();
		this->resize(startSize + length);
		memcpy(this->getDataOffset(startSize), data, length);
	}

	void *getData() { return data.data(); }

	void clear() { data.clear(); }

	void *getDataOffset(size_t offset) { return &(data.data()[offset]); }
};

void PCK::loadVersion1Format(IFile &pck, IFile &tab, int Index)
{
	sp<PaletteImage> img;

	uint16_t c0_offset;
	uint16_t c0_maxwidth;
	uint16_t c0_height;
	std::unique_ptr<Memory> c0_imagedata;
	size_t c0_bufferptr;
	int c0_idx;
	std::vector<int16_t> c0_rowwidths;

	int minrec = (Index < 0 ? 0 : Index);
	int maxrec = (Index < 0 ? tab.size() / 4 : Index + 1);
	for (int i = minrec; i < maxrec; i++)
	{
		if (!tab.seekg(i * 4, std::ios::beg))
		{
			LogError("Failed to seek to record %d in \"%s\"", i, tab.fileName().cStr());
			return;
		}
		unsigned int offset;
		if (!tab.readule32(offset))
		{
			LogError("Failed to read offset %d from tab \"%s\"", i, tab.fileName().cStr());
			return;
		}

		if (!pck.seekg(offset, std::ios::beg))
		{
			LogError("Failed to seek to offset %u for PCK \"%s\" id %d", offset,
			         pck.fileName().cStr(), i);
			return;
		}

		// Raw Data
		if (!pck.readule16(c0_offset))
		{
			LogError("Failed to read offset header in PCK \"%s\" id %d", pck.fileName().cStr(), i);
			return;
		}
		c0_imagedata.reset(new Memory(0));
		c0_rowwidths.clear();
		c0_maxwidth = 0;
		c0_height = 0;

		while (c0_offset != 0xffff)
		{
			uint16_t c0_width;
			if (!pck.readule16(c0_width))
			{
				LogError("Failed to read width header in PCK \"%s\" id %d", pck.fileName().cStr(),
				         i);
				return;
			}
			c0_rowwidths.push_back(c0_width);
			if (c0_maxwidth < c0_width)
			{
				c0_maxwidth = c0_width;
			}

			c0_bufferptr = c0_imagedata->getSize();
			c0_imagedata->resize(c0_bufferptr + c0_width + (c0_offset % 640));
			memset(c0_imagedata->getDataOffset(c0_bufferptr), 0, c0_width + (c0_offset % 640));
			if (!pck.read(reinterpret_cast<char *>(
			                  c0_imagedata->getDataOffset(c0_bufferptr + (c0_offset % 640))),
			              c0_width))
			{
				LogError("Failed to read pixel data in PCK \"%s\" id %d", pck.fileName().cStr(), i);
				return;
			}
			c0_height++;

			if (!pck.readule16(c0_offset))
			{
				LogError("Failed to read offset after %d from tab \"%s\"", i,
				         tab.fileName().cStr());
				return;
			}
		}
		img = mksp<PaletteImage>(Vec2<int>{c0_maxwidth, c0_height});
		PaletteImageLock region(img);
		c0_idx = 0;
		for (int c0_y = 0; c0_y < c0_height; c0_y++)
		{
			for (int c0_x = 0; c0_x < c0_maxwidth; c0_x++)
			{
				if (c0_x < c0_rowwidths.at(c0_y))
				{
					region.set(Vec2<int>{c0_x, c0_y},
					           (reinterpret_cast<char *>(c0_imagedata->getDataOffset(c0_idx)))[0]);
				}
				else
				{
					region.set(Vec2<int>{c0_x, c0_y}, 0);
				}
				c0_idx++;
			}
		}
		images.push_back(img);
	}
}

void PCK::loadVersion2Format(IFile &pck, IFile &tab, int Index)
{
	uint16_t compressionmethod;

	sp<PaletteImage> img;

	PCKCompression1ImageHeader c1_imgheader;
	uint32_t c1_pixelstoskip;
	PCKCompression1RowHeader c1_header;

	int minrec = (Index < 0 ? 0 : Index);
	int maxrec = (Index < 0 ? tab.size() / 4 : Index + 1);
	for (int i = minrec; i < maxrec; i++)
	{
		if (!tab.seekg(i * 4, std::ios::beg))
		{
			LogError("Failed to seek to record %d in \"%s\"", i, tab.fileName().cStr());
			return;
		}
		unsigned int offset;
		if (!tab.readule32(offset))
		{
			LogError("Failed to read offset %d from tab \"%s\"", i, tab.fileName().cStr());
			return;
		}
		offset *= 4;

		if (!pck.seekg(offset, std::ios::beg))
		{
			LogError("Failed to seek to offset %u for PCK \"%s\" id %d", offset,
			         pck.fileName().cStr(), i);
			return;
		}

		if (!pck.readule16(compressionmethod))
		{
			LogError("Failed to read compression header for PCK \"%s\" id %d",
			         pck.fileName().cStr(), i);
			return;
		}
		switch (compressionmethod)
		{
			case 0:

				break;

			case 1:
			{
				// Raw Data with RLE
				if (!pck.read(reinterpret_cast<char *>(&c1_imgheader), sizeof(c1_imgheader)))
				{
					LogError("Failed to read header for PCK \"%s\" id %d", pck.fileName().cStr(),
					         i);
					return;
				}
				img = mksp<PaletteImage>(
				    Vec2<int>{c1_imgheader.RightMostPixel, c1_imgheader.BottomMostPixel});

				PaletteImageLock lock(img);
				if (!pck.readule32(c1_pixelstoskip))
				{
					LogError("Failed to read pixel skip for PCK \"%s\" id %d",
					         pck.fileName().cStr(), i);
					return;
				}
				while (c1_pixelstoskip != 0xFFFFFFFF)
				{
					if (!pck.read(reinterpret_cast<char *>(&c1_header), sizeof(c1_header)))
					{
						LogError("Failed to read RLE header for PCK \"%s\" id %d",
						         pck.fileName().cStr(), i);
						return;
					}
					uint32_t c1_y = (c1_pixelstoskip / 640);

					if (c1_y < c1_imgheader.BottomMostPixel)
					{
						if (c1_header.BytesInRow != 0)
						{
							// No idea what this is
							uint32_t chunk;
							pck.readule32(chunk);

							for (uint32_t c1_x = c1_imgheader.LeftMostPixel;
							     c1_x < c1_header.BytesInRow; c1_x++)
							{
								if (c1_x < c1_imgheader.RightMostPixel)
								{
									char idx;
									if (!pck.read(&idx, 1))
									{
										LogError("Failed to read pixel data for PCK \"%s\" id %d",
										         pck.fileName().cStr(), i);
										return;
									}
									lock.set(Vec2<int>{c1_x, c1_y}, idx);
								}
								else
								{
									// Pretend to process data
									char dummy;
									pck.read(&dummy, 1);
								}
							}
						}
						else
						{
							for (uint32_t c1_x = 0; c1_x < c1_header.PixelsInRow; c1_x++)
							{
								if ((c1_header.ColumnToStartAt + c1_x) <
								    c1_imgheader.RightMostPixel)
								{
									char idx;
									if (!pck.read(&idx, 1))
									{
										LogError("Failed to read pixel data for PCK \"%s\" id %d",
										         pck.fileName().cStr(), i);
										return;
									}
									lock.set(Vec2<int>{c1_header.ColumnToStartAt + c1_x, c1_y},
									         idx);
								}
								else
								{
									// Pretend to process data
									char dummy;
									pck.read(&dummy, 1);
								}
							}
						}
					}
					if (!pck.readule32(c1_pixelstoskip))
					{
						LogError("Failed to read pixel skip after PCK \"%s\" id %d",
						         pck.fileName().cStr(), i);
						return;
					}
				}
				images.push_back(img);
				break;
			}

			default:
				LogError("Unsupported compression method %d", compressionmethod);
				break;
		}
	}
}
}; // anonymous namespace

sp<ImageSet> PCKLoader::load(Data &data, UString PckFilename, UString TabFilename)
{
	PCK *p = new PCK(data, PckFilename, TabFilename);
	auto imageSet = mksp<ImageSet>();
	imageSet->maxSize = Vec2<int>{0, 0};
	imageSet->images.resize(p->images.size());
	for (unsigned int i = 0; i < p->images.size(); i++)
	{
		p->images[i]->calculateBounds();
		imageSet->images[i] = p->images[i];
		imageSet->images[i]->owningSet = imageSet;
		imageSet->images[i]->indexInSet = i;
		if (imageSet->images[i]->size.x > imageSet->maxSize.x)
			imageSet->maxSize.x = imageSet->images[i]->size.x;
		if (imageSet->images[i]->size.y > imageSet->maxSize.y)
			imageSet->maxSize.y = imageSet->images[i]->size.y;
	}
	delete p;

	LogInfo("Loaded \"%s\" - %u images, max size {%d,%d}", PckFilename.cStr(),
	        static_cast<unsigned int>(imageSet->images.size()), imageSet->maxSize.x,
	        imageSet->maxSize.y);

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
				LogWarning("Writing to {%d,%d} in 8x8 stratmap image", x, y);
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
		LogWarning("Failed to open tab \"%s\"", TabFilename.cStr());
		return nullptr;
	}
	auto pckFile = data.fs.open(PckFilename);
	if (!pckFile)
	{
		LogWarning("Failed to open tab \"%s\"", TabFilename.cStr());
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
                                   uint8_t shadedIdx)
{
	TRACE_FN;
	auto imageSet = mksp<ImageSet>();
	auto tabFile = data.fs.open(TabFilename);
	if (!tabFile)
	{
		LogWarning("Failed to open tab \"%s\"", TabFilename.cStr());
		return nullptr;
	}
	auto pckFile = data.fs.open(PckFilename);
	if (!pckFile)
	{
		LogWarning("Failed to open tab \"%s\"", TabFilename.cStr());
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
