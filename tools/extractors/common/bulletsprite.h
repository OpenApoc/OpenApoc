#pragma once

#include <cstdint>

struct BulletSprite
{
	uint8_t sprite[3][3]; // A 3x3 paletted image
};

static_assert(sizeof(struct BulletSprite) == 9, "Invalid bullet_sprite size");

struct ProjectileSprites
{
	uint8_t sprites[36]; // Index into bullet_sprites[] - 255 == no sprite
};

static_assert(sizeof(struct ProjectileSprites) == 36, "Invalid projectile_sprites size");

#define BULLETSPRITE_DATA_UFO2P_OFFSET_START 0x18B660
#define BULLETSPRITE_DATA_UFO2P_OFFSET_END 0x18B7A4
static_assert(BULLETSPRITE_DATA_UFO2P_OFFSET_START + 36 * sizeof(BulletSprite) ==
                  BULLETSPRITE_DATA_UFO2P_OFFSET_END,
              "Should be 36 entries in bulletsprite table?");

#define PROJECTILESPRITES_DATA_UFO2P_OFFSET_START 0x18B7A4
#define PROJECTILESPRITES_DATA_UFO2P_OFFSET_END                                                    \
	(PROJECTILESPRITES_DATA_UFO2P_OFFSET_START + 15 * sizeof(ProjectileSprites))

#define BULLETSPRITE_DATA_TACP_OFFSET_START 2105940
#define BULLETSPRITE_DATA_TACP_OFFSET_END                                                          \
	(BULLETSPRITE_DATA_TACP_OFFSET_START + 58 * sizeof(BulletSprite))

#define PROJECTILESPRITES_DATA_TACP_OFFSET_START 2106462
#define PROJECTILESPRITES_DATA_TACP_OFFSET_END                                                     \
	(PROJECTILESPRITES_DATA_TACP_OFFSET_START + 14 * sizeof(ProjectileSprites))
