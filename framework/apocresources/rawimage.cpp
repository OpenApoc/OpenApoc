#include "framework/apocresources/rawimage.h"
#include "framework/data.h"
#include "framework/image.h"
#include "framework/logger.h"
#include "library/sp.h"

namespace OpenApoc
{

sp<PaletteImage> RawImage::load(Data &data, const UString &filename, const Vec2<int> &size)
{
	auto infile = data.fs.open(filename);
	if (!infile)
	{
		LogWarning("Failed to open file \"{}\"", filename);
		return nullptr;
	}
	if (size.x <= 0 || size.y <= 0)
	{
		LogWarning("Trying to read image of invalid size {{{},{}}}", size.x, size.y);
		return nullptr;
	}

	if (infile.size() != static_cast<size_t>(size.x * size.y))
	{
		LogWarning("File \"{}\" has incorrect size for raw image of size {}", filename, size);
	}

	auto image = mksp<PaletteImage>(size);

	PaletteImageLock l(image, ImageLockUse::Write);

	for (int y = 0; y < size.y; y++)
	{
		for (int x = 0; x < size.x; x++)
		{
			// TODO: can read a stride at a time? Or just the whole chunk as it's tightly packed?
			uint8_t idx;
			if (!infile.read(reinterpret_cast<char *>(&idx), 1))
			{
				LogError("Unexpected EOF in file \"{}\" at {{{},{}}}", filename, x, y);
				return nullptr;
			}
			l.set(Vec2<unsigned int>{x, y}, idx);
		}
	}

	return image;
}

sp<ImageSet> RawImage::loadSet(Data &data, const UString &filename, const Vec2<int> &size)
{
	auto infile = data.fs.open(filename);
	if (!infile)
	{
		LogWarning("Failed to open file \"{}\"", filename);
		return nullptr;
	}
	if (size.x <= 0 || size.y <= 0)
	{
		LogWarning("Trying to read images of invalid size {}", size);
		return nullptr;
	}

	if (infile.size() % static_cast<size_t>(size.x * size.y) != 0)
	{
		LogWarning("File \"{}\" has incorrect size for raw images of size {}", filename, size);
	}

	size_t numImages = infile.size() / (size.x * size.y);
	auto imageSet = mksp<ImageSet>();
	imageSet->maxSize = size;
	imageSet->images.resize(numImages);

	for (size_t i = 0; i < numImages; i++)
	{
		auto image = mksp<PaletteImage>(size);

		PaletteImageLock l(image, ImageLockUse::Write);

		for (int y = 0; y < size.y; y++)
		{
			for (int x = 0; x < size.x; x++)
			{
				// TODO: can read a stride at a time? Or just the whole chunk as it's tightly
				// packed?
				uint8_t idx;
				if (!infile.read(reinterpret_cast<char *>(&idx), 1))
				{
					LogError("Unexpected EOF in file \"{}\" at {{{}:{},{}}}", filename, i, x, y);
					return nullptr;
				}
				l.set(Vec2<unsigned int>{x, y}, idx);
			}
		}

		imageSet->images[i] = image;
		imageSet->images[i]->owningSet = imageSet;
		imageSet->images[i]->indexInSet = i;
	}

	return imageSet;
}

}; // namespace OpenApoc
