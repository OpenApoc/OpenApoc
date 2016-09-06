#include "framework/framework.h"
#include "framework/logger.h"

using namespace OpenApoc;

static bool testImage(const UString &imageName, const UString &referenceName)
{
	auto img = fw().data->loadImage(imageName);
	auto reference = fw().data->loadImage(referenceName);
	if (!img)
	{
		LogWarning("Failed to load image");
		return false;
	}
	if (!reference)
	{
		LogWarning("Failed to load reference");
		return false;
	}

	auto rgbImg = std::dynamic_pointer_cast<RGBImage>(img);
	if (!rgbImg)
	{
		LogWarning("Image not RGBImage");
		return false;
	}

	auto rgbReference = std::dynamic_pointer_cast<RGBImage>(reference);
	if (!rgbReference)
	{
		LogWarning("Reference not RGB image");
		return false;
	}

	if (img->size != reference->size)
	{
		LogWarning("Invalid size, {%d,%d} doesn't match reference {%d,%d}", img->size.x,
		           img->size.y, reference->size.x, reference->size.y);
		return false;
	}

	RGBImageLock imgLock(rgbImg, ImageLockUse::Read);
	RGBImageLock refLock(rgbReference, ImageLockUse::Read);

	for (unsigned int y = 0; y < img->size.y; y++)
	{
		for (unsigned int x = 0; x < img->size.x; x++)
		{
			auto i = imgLock.get({x, y});
			auto r = refLock.get({x, y});
			if (i != r)
			{
				LogWarning(
				    "Image mismatch at {%d,%d} (RGBA img {%d,%d,%d,%d} != RGBA ref {%d,%d,%d,%d}",
				    x, y, (int)i.r, (int)i.g, (int)i.b, (int)i.a, (int)r.r, (int)r.g, (int)r.b,
				    (int)r.a);

				return false;
			}
		}
	}
	return true;
}

int main(int, char **)
{
	std::map<UString, UString> testImages = {
	    // PCX files:
	    {"xcom3/UFODATA/TITLES.PCX", "test_images/ufodata_titles.png"},
	    // RAW files:
	    {"RAW:xcom3/ufodata/isobord1.dat:640:128:xcom3/ufodata/pal_01.dat",
	     "test_images/ufodata_isobord1_pal01.png"},
	    // PCK files:
	    {"PCK:xcom3/UFODATA/NEWBUT.PCK:xcom3/UFODATA/NEWBUT.TAB:30:xcom3/UFODATA/BASE.PCX",
	     "test_images/ufodata_newbut_3_base_pcx.png"},
	    {"PCK:xcom3/UFODATA/city.PCK:xcom3/UFODATA/city.TAB:956:xcom3/UFODATA/pal_01.dat",
	     "test_images/ufodata_city_956_pal01.png"},
	    // PCKSTRAT files:
	    {"PCKSTRAT:xcom3/UFODATA/stratmap.PCK:xcom3/UFODATA/stratmap.TAB:32:xcom3/UFODATA/"
	     "pal_01.dat",
	     "test_images/ufodata_stratmap_32.png"},
	    // SHADOW files:
	    {"PCKSHADOW:xcom3/UFODATA/SHADOW.PCK:xcom3/UFODATA/SHADOW.TAB:5:xcom3/UFODATA/pal_01.dat",
	     "test_images/ufodata_shadow_5.png"},
	};
	Framework fw("OpenApoc", {}, false);

	for (auto &imagePair : testImages)
	{
		if (!testImage(imagePair.first, imagePair.second))
		{
			LogError("Image \"%s\" didn't match reference \"%s\"", imagePair.first.cStr(),
			         imagePair.second.cStr());
			return EXIT_FAILURE;
		}
		LogInfo("Image \"%s\" matches reference \"%s\"", imagePair.first.cStr(),
		        imagePair.second.cStr());
	}

	return EXIT_SUCCESS;
}
