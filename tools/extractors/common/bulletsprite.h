#pragma once
struct bullet_sprite_t
{
	uint8_t sprite[3][3]; // A 3x3 paletted image
};

static_assert(sizeof(struct bullet_sprite_t) == 9, "Invalid bullet_sprite size");

struct projectile_sprites_t
{
	uint8_t sprites[36]; // Index into bullet_sprites[] - 255 == no sprite
};

static_assert(sizeof(struct projectile_sprites_t) == 36, "Invalid projectile_sprites size");

#define BULLETSPRITE_DATA_OFFSET_START 0x18B660
#define BULLETSPRITE_DATA_OFFSET_END 0x18B7A4
static_assert(BULLETSPRITE_DATA_OFFSET_START + 36 * sizeof(bullet_sprite_t) ==
                  BULLETSPRITE_DATA_OFFSET_END,
              "Should be 36 entries in bulletsprite table?");

#define PROJECTILESPRITES_DATA_OFFSET_START 0x18B7A4
#define PROJECTILESPRITES_DATA_OFFSET_END                                                          \
	(PROJECTILESPRITES_DATA_OFFSET_START + 15 * sizeof(projectile_sprites_t))
