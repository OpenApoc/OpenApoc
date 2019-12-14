#include "framework/configfile.h"
#include "framework/logger.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tilemap.h"
#include "library/voxel.h"
#include <array>
#include <utility>
#include <vector>

using namespace OpenApoc;

class FakeSceneryTileObject : public TileObject
{
  public:
	Vec3<float> position;
	sp<VoxelMap> voxel;

	Vec3<float> getPosition() const override { return this->position; }
	bool hasVoxelMap(bool los [[maybe_unused]]) const override { return true; }
	sp<VoxelMap> getVoxelMap(Vec3<int>, bool) const override { return this->voxel; }

	FakeSceneryTileObject(TileMap &map, Vec3<float> bounds, sp<VoxelMap> voxelMap)
	    : TileObject(map, Type::Scenery, bounds), position(0, 0, 0), voxel(voxelMap)
	{
		this->name = "FAKE_SCENERY";
	}
	~FakeSceneryTileObject() override = default;

	void setPosition(Vec3<float> newPosition) override
	{
		this->position = newPosition;
		TileObject::setPosition(newPosition);
	}
	void draw(Renderer &, TileTransform &, Vec2<float>, TileViewMode, bool, int, bool,
	          bool) override
	{
		LogError("DRAW CALLED ON FAKE SCENERY??");
		exit(EXIT_FAILURE);
	}
};

static void test_collision(const TileMap &map, Vec3<float> line_start, Vec3<float> line_end,
                           sp<TileObject> expected_collision)
{
	auto collision = map.findCollision(line_start, line_end);
	if (collision.obj != expected_collision)
	{
		LogError("Line between {%f,%f,%f} and {%f,%f,%f} collided with %s, expected %s",
		         line_start.x, line_start.y, line_start.z, line_end.x, line_end.y, line_end.z,
		         collision.obj ? collision.obj->getName() : "NONE",
		         expected_collision ? expected_collision->getName() : "NONE");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}
	auto filled_slice_32_32 = mksp<VoxelSlice>(Vec2<int>{32, 32});
	for (int y = 0; y < 32; y++)
	{
		for (int x = 0; x < 32; x++)
		{
			filled_slice_32_32->setBit({x, y}, true);
		}
	}

	auto empty_tilemap_32_32_16 = mksp<VoxelMap>(Vec3<int>{32, 32, 16});
	auto filled_tilemap_32_32_16 = mksp<VoxelMap>(Vec3<int>{32, 32, 16});
	for (int z = 0; z < 16; z++)
	{
		filled_tilemap_32_32_16->setSlice(z, filled_slice_32_32);
	}

	// LayerMap is only used for drawing
	TileMap map{{100, 100, 10}, {1, 1, 1}, {32, 32, 16}, {{TileObject::Type::Scenery}}};

	// Spawn some objects
	std::vector<std::pair<Vec3<float>, sp<TileObject>>> objects = {
	    {Vec3<float>{2.5, 2.5, 2.5},
	     mksp<FakeSceneryTileObject>(map, Vec3<float>{1, 1, 1}, filled_tilemap_32_32_16)},
	    {Vec3<float>{99.5, 99.5, 9.5},
	     mksp<FakeSceneryTileObject>(map, Vec3<float>{1, 1, 1}, filled_tilemap_32_32_16)},
	};

	for (auto &object : objects)
	{
		auto initialPosition = object.first;
		object.second->setPosition(initialPosition);
		if (initialPosition != object.second->getPosition())
		{
			LogError("Object %s moved from {%f,%f,%f} to {%f,%f,%f}", object.second->getName(),
			         initialPosition.x, initialPosition.y, initialPosition.z,
			         object.second->getPosition().x, object.second->getPosition().y,
			         object.second->getPosition().z);
			exit(EXIT_FAILURE);
		}
	}

	// Compare some expected collisions
	//{{line_start,line_end},expected_object}
	std::vector<std::pair<std::array<Vec3<float>, 2>, sp<TileObject>>> collisions = {
	    {{{Vec3<float>{0, 0, 0}, Vec3<float>{1, 1, 1}}}, nullptr},
	    {{{Vec3<float>{2.1, 2.1, 0}, Vec3<float>{2.1, 2.1, 4}}}, objects[0].second},
	    {{{Vec3<float>{2.6, 2.6, 0}, Vec3<float>{2.6, 2.6, 4}}}, objects[0].second},
	    {{{Vec3<float>{2.6, 0, 2.1}, Vec3<float>{2.6, 4, 2.1}}}, objects[0].second},
	    {{{Vec3<float>{2.6, 0, 2.6}, Vec3<float>{2.6, 4, 2.6}}}, objects[0].second},
	    {{{Vec3<float>{0, 2.1, 2.1}, Vec3<float>{4, 2.1, 2.1}}}, objects[0].second},
	    {{{Vec3<float>{0, 2.6, 2.6}, Vec3<float>{4, 2.6, 2.6}}}, objects[0].second},
	    {{{Vec3<float>{2.1, 2.1, 0}, Vec3<float>{2.1, 2.6, 4}}}, objects[0].second},
	    {{{Vec3<float>{2.6, 2.6, 0}, Vec3<float>{2.1, 2.6, 4}}}, objects[0].second},
	    {{{Vec3<float>{2.6, 0, 2.1}, Vec3<float>{2.1, 4, 2.1}}}, objects[0].second},
	    {{{Vec3<float>{2.1, 0, 2.6}, Vec3<float>{2.6, 4, 2.6}}}, objects[0].second},
	    {{{Vec3<float>{0, 2.6, 2.6}, Vec3<float>{4, 2.1, 2.6}}}, objects[0].second},
	    {{{Vec3<float>{0, 2.1, 2.6}, Vec3<float>{4, 2.6, 2.6}}}, objects[0].second},
	};

	for (auto &collision : collisions)
	{
		test_collision(map, collision.first[0], collision.first[1], collision.second);
	}

	return EXIT_SUCCESS;
}
