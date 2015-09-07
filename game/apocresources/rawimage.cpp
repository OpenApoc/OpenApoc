#include "game/apocresources/rawimage.h"
#include "framework/logger.h"
#include "framework/data.h"
#include "framework/image.h"

namespace OpenApoc
{

std::shared_ptr<PaletteImage> RawImage::load(Data &data, const UString &filename,
                                             const Vec2<int> &size)
{
	auto infile = data.load_file(filename);
	if (!infile) {
		LogWarning("Failed to open file \"%s\"", filename.str().c_str());
		return nullptr;
	}
	if (size.x <= 0 || size.y <= 0) {
		LogWarning("Trying to read image of invalid size {%d,%d}", size.x, size.y);
		return nullptr;
	}

	if (infile.size() != (size_t)(size.x * size.y)) {
		LogWarning("File \"%s\" has incorrect size for raw image of size {%d,%d}",
		           filename.str().c_str(), size.x, size.y);
	}

	auto image = std::make_shared<PaletteImage>(size);

	PaletteImageLock l(image, ImageLockUse::Write);

	for (int y = 0; y < size.y; y++) {
		for (int x = 0; x < size.x; x++) {
			// TODO: can read a stride at a time? Or just the whole chunk as it's tightly packed?
			uint8_t idx;
			if (!infile.read((char *)&idx, 1)) {
				LogError("Unexpected EOF in file \"%s\" at {%d,%d}", filename.str().c_str(), x, y);
				return nullptr;
			}
			l.set(Vec2<unsigned int>{x, y}, idx);
		}
	}

	return image;
}

} // namespace OpenApoc
