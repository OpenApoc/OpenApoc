#include "framework/framework.h"
#include "game/state/rules/city/baselayout.h"
#include "library/strings_format.h"
#include "tools/extractors/common/ufo2p.h"
#include "tools/extractors/extractors.h"

namespace OpenApoc
{

std::map<UString, sp<Image>> InitialGameStateExtractor::extractBulletSpritesCity() const
{
	std::map<UString, sp<Image>> sprites;
	auto &data = this->ufo2p;

	UString path_prefix = "bulletsprites/city/";

	for (unsigned i = 0; i < data.bullet_sprites->count(); i++)
	{
		UString path = fmt::format("{}{:02}.png", path_prefix, i);
		auto sprite = data.bullet_sprites->get(i);
		auto img = mksp<PaletteImage>(Vec2<unsigned int>{3, 3});
		PaletteImageLock l(img);

		for (int x = 0; x < 3; x++)
		{
			for (int y = 0; y < 3; y++)
			{
				l.set({x, y}, sprite.sprite[y][x]);
			}
		}
		sprites[path] = img;
	}

	return sprites;
}

std::map<UString, sp<Image>> InitialGameStateExtractor::extractBulletSpritesBattle() const
{
	std::map<UString, sp<Image>> sprites;
	auto &data = this->tacp;

	UString path_prefix = "bulletsprites/battle/";

	for (unsigned i = 0; i < data.bullet_sprites->count(); i++)
	{
		UString path = fmt::format("{}{:02}.png", path_prefix, i);
		auto sprite = data.bullet_sprites->get(i);
		auto img = mksp<PaletteImage>(Vec2<unsigned int>{3, 3});
		PaletteImageLock l(img);

		for (int x = 0; x < 3; x++)
		{
			for (int y = 0; y < 3; y++)
			{
				l.set({x, y}, sprite.sprite[y][x]);
			}
		}
		sprites[path] = img;
	}

	return sprites;
}

} // namespace OpenApoc
