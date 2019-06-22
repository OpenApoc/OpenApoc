#include "framework/configfile.h"
#include "framework/logger.h"
#include "library/rect.h"

using namespace OpenApoc;

// This is a rather limited test, as we don't want to encode /how/ the set is collapsed in a test
// so we just check the initial size is as expected (IE set.insert() didn't screw up), at least one
// rect is expected to be collapsed, we never collapse down to 0, and we never run the collapse loop
// more tiles than the total number of rects (As we can never collapse more rects than exist)
template <typename T>
static bool test_one_rect_compaction(std::set<Rect<T>> rect_set, unsigned expected_start_count,
                                     bool expected_to_collapse, unsigned expected_end_size = 0)
{
	if (rect_set.size() != expected_start_count)
	{
		LogError("Rect set has size %u at start, expected %u", (unsigned)rect_set.size(),
		         expected_start_count);
		return false;
	}
	unsigned num_collapsed = Rect<T>::compactRectSet(rect_set);

	if (expected_start_count != 0 && rect_set.empty())
	{
		LogError("Collapsed down to zero size set");
		return false;
	}
	if (num_collapsed && num_collapsed >= expected_start_count)
	{
		LogError("Somehow managed to collapse %u rects in a set containing %u rects", num_collapsed,
		         expected_start_count);
		return false;
	}
	if (Rect<T>::compactRectSet(rect_set) != 0)
	{
		LogError("A second collapse actually collapsed something?");
		return false;
	}

	if (expected_to_collapse && num_collapsed == 0)
	{
		LogError("No rects collapsed but some were expected");
		return false;
	}

	if (expected_end_size && rect_set.size() != expected_end_size)
	{
		LogError("Expected to collapse to %u rects but got %u", (unsigned)rect_set.size(),
		         expected_end_size);
		return false;
	}

	return true;
}

static bool test_rect_compaction()
{
	std::set<Rect<int>> rect_set;

	if (!test_one_rect_compaction(rect_set, 0, false))
	{
		LogError("zero-sized set failed");
		return false;
	}
	rect_set.clear();
	rect_set.insert(Rect<int>{{0, 0}, {1, 1}});
	if (!test_one_rect_compaction(rect_set, 1, false))
	{
		LogError("one-sized set failed");
		return false;
	}

	rect_set.clear();
	rect_set.insert(Rect<int>{{0, 0}, {1, 1}});
	rect_set.insert(Rect<int>{{1, 0}, {2, 1}});

	if (!test_one_rect_compaction(rect_set, 2, true))
	{
		LogError("trivial x compact set failed");
		return false;
	}

	rect_set.clear();
	rect_set.insert(Rect<int>{{0, 0}, {1, 1}});
	rect_set.insert(Rect<int>{{0, 1}, {1, 2}});

	if (!test_one_rect_compaction(rect_set, 2, true))
	{
		LogError("trivial y compact set failed");
		return false;
	}

	rect_set.clear();
	rect_set.insert(Rect<int>{{0, 0}, {1, 1}});
	rect_set.insert(Rect<int>{{1, 1}, {2, 2}});

	if (!test_one_rect_compaction(rect_set, 2, false))
	{
		LogError("trivial non-compactable set failed");
		return false;
	}

	rect_set.clear();
	rect_set.insert(Rect<int>{{0, 0}, {1, 1}});
	rect_set.insert(Rect<int>{{1, 1}, {2, 2}});

	if (!test_one_rect_compaction(rect_set, 2, false))
	{
		LogError("trivial non-compactable set failed");
		return false;
	}

	rect_set.clear();
	rect_set.insert(Rect<int>{{0, 0}, {1, 1}});
	rect_set.insert(Rect<int>{{1, 0}, {2, 1}});
	rect_set.insert(Rect<int>{{3, 3}, {4, 4}});

	if (!test_one_rect_compaction(rect_set, 3, true, 2))
	{
		LogError("3->2 x compaction set failed");
		return false;
	}

	rect_set.clear();
	rect_set.insert(Rect<int>{{0, 0}, {1, 1}});
	rect_set.insert(Rect<int>{{0, 1}, {1, 2}});
	rect_set.insert(Rect<int>{{3, 3}, {4, 4}});

	if (!test_one_rect_compaction(rect_set, 3, true, 2))
	{
		LogError("3->2 y compaction set failed");
		return false;
	}

	return true;
}

void test_point_within(Rect<int> r, Vec2<int> p, bool expected)
{
	if (r.within(p) != expected)
	{
		LogError("Point %s incorrectly %s rect %s", p, expected ? "not within" : "within", r);
		exit(EXIT_FAILURE);
	}
}
void test_rect_within(Rect<int> r1, Rect<int> r2, bool expected)
{
	if (r1.within(r2) != expected)
	{
		LogError("Rect %s incorrectly %s rect %s", r2, expected ? "not within" : "within", r1);
		exit(EXIT_FAILURE);
	}
}
void test_rect_intersects(Rect<int> r1, Rect<int> r2, bool expected)
{
	if (r1.intersects(r2) != expected)
	{
		LogError("Rect %s incorrectly %s rect %s", r2,
		         expected ? "does not intersect" : "intersects", r1);
		exit(EXIT_FAILURE);
	}
}
int main(int argc, char **argv)
{
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}
	test_point_within({0, 0, 1, 1}, {0, 0}, true);

	test_point_within({0, 0, 3, 3}, {1, 0}, true);
	test_point_within({0, 0, 3, 3}, {2, 1}, true);
	test_point_within({0, 0, 3, 3}, {2, 2}, true);
	test_point_within({0, 0, 3, 3}, {1, 2}, true);

	test_point_within({0, 0, 1, 1}, {1, 1}, false);
	test_point_within({0, 0, 1, 1}, {0, 1}, false);
	test_point_within({0, 0, 1, 1}, {-1, 1}, false);
	test_point_within({0, 0, 1, 1}, {-1, 0}, false);
	test_point_within({0, 0, 1, 1}, {-1, -1}, false);
	test_point_within({0, 0, 1, 1}, {0, -1}, false);
	test_point_within({0, 0, 1, 1}, {1, -1}, false);
	test_point_within({0, 0, 1, 1}, {1, 0}, false);

	test_rect_within({0, 0, 1, 1}, {0, 0, 1, 1}, true);

	test_rect_within({0, 0, 1, 1}, {1, 0, 2, 1}, false);
	test_rect_within({0, 0, 1, 1}, {0, 1, 1, 2}, false);
	test_rect_within({0, 0, 1, 1}, {-1, 0, 1, 1}, false);
	test_rect_within({0, 0, 1, 1}, {0, -1, 1, 1}, false);

	test_rect_intersects({0, 0, 1, 1}, {0, 0, 1, 1}, true);
	test_rect_intersects({0, 0, 1, 1}, {0, 0, 2, 1}, true);
	test_rect_intersects({0, 0, 1, 1}, {0, 0, 1, 2}, true);
	test_rect_intersects({0, 0, 1, 1}, {-1, 0, 1, 1}, true);
	test_rect_intersects({0, 0, 1, 1}, {0, -1, 1, 1}, true);

	test_rect_intersects({0, 0, 1, 1}, {0, 1, 1, 2}, false);
	test_rect_intersects({0, 0, 1, 1}, {1, 0, 2, 1}, false);
	test_rect_intersects({0, 0, 1, 1}, {-1, 0, 0, 1}, false);
	test_rect_intersects({0, 0, 1, 1}, {0, -1, 1, 0}, false);
	if (!test_rect_compaction())
	{
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
