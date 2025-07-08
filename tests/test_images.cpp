#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/logger.h"
#include <map>

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
		LogWarning("Invalid size, {} doesn't match reference {}", img->size, reference->size);
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
				LogWarning("Image mismatch at {{{},{}}} (RGBA img {{{},{},{},{}}} != RGBA ref "
				           "{{{},{},{},{}}}",
				           x, y, (int)i.r, (int)i.g, (int)i.b, (int)i.a, (int)r.r, (int)r.g,
				           (int)r.b, (int)r.a);

				return false;
			}
		}
	}
	return true;
}

int main(int argc, char **argv)
{
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}
	std::map<UString, UString> testImages = {
	    // PCX files:
	    {"xcom3/ufodata/titles.pcx", "test_images/ufodata_titles.png"},
	    // RAW files:
	    {"RAW:xcom3/ufodata/isobord1.dat:640:128:xcom3/ufodata/pal_01.dat",
	     "test_images/ufodata_isobord1_pal01.png"},
	    // PCK files:
	    {"PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:30:xcom3/ufodata/base.pcx",
	     "test_images/ufodata_newbut_3_base_pcx.png"},
	    {"PCK:xcom3/ufodata/city.pck:xcom3/ufodata/city.tab:956:xcom3/ufodata/pal_01.dat",
	     "test_images/ufodata_city_956_pal01.png"},
	    {"PCK:xcom3/tacdata/unit/xcom1a.pck:xcom3/tacdata/unit/xcom1a.tab:30:xcom3/tacdata/"
	     "tactical.pal",
	     "test_images/tacdata_unit_xcom1a_30_tactical.png"},
	    {"PCK:xcom3/tacdata/unit/xcom1a.pck:xcom3/tacdata/unit/xcom1a.tab:240:xcom3/tacdata/"
	     "tactical.pal",
	     "test_images/tacdata_unit_xcom1a_240_tactical.png"},
	    {"PCK:xcom3/tacdata/unit/xcom1a.pck:xcom3/tacdata/unit/xcom1a.tab:134:xcom3/tacdata/"
	     "tactical.pal",
	     "test_images/tacdata_unit_xcom1a_134_tactical.png"},
	    // PCKSTRAT files:
	    {"PCKSTRAT:xcom3/ufodata/stratmap.pck:xcom3/ufodata/stratmap.tab:32:xcom3/ufodata/"
	     "pal_01.dat",
	     "test_images/ufodata_stratmap_32.png"},
	    // SHADOW files:
	    {"PCKSHADOW:xcom3/ufodata/shadow.pck:xcom3/ufodata/shadow.tab:5:xcom3/ufodata/pal_01.dat",
	     "test_images/ufodata_shadow_5.png"},
	    // LOFTEMPS files:
	    {"LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/ufodata/loftemps.tab:113",
	     "test_images/ufodata_loftemps_113.png"},
	    {"LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/ufodata/loftemps.tab:150",
	     "test_images/ufodata_loftemps_150.png"},
	    {"LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/ufodata/loftemps.tab:151",
	     "test_images/ufodata_loftemps_151.png"},
	};
	Framework fw("OpenApoc", false);

	for (auto &imagePair : testImages)
	{
		if (!testImage(imagePair.first, imagePair.second))
		{
			LogError("Image \"{}\" didn't match reference \"{}\"", imagePair.first,
			         imagePair.second);
			return EXIT_FAILURE;
		}
		LogInfo("Image \"{}\" matches reference \"{}\"", imagePair.first, imagePair.second);
	}

	return EXIT_SUCCESS;
}
