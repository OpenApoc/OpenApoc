#include "library/rect.h"
#include "framework/logger.h"

using namespace OpenApoc;

void test_point_within(Rect<int> r, Vec2<int> p, bool expected)
{
	if (r.within(p) != expected)
	{
		LogError("Point {%d,%d} incorrectly %s rect {%d,%d},{%d,%d}", p.x, p.y,
		         expected ? "not within" : "within", r.p0.x, r.p0.y, r.p1.x, r.p1.y);
		exit(EXIT_FAILURE);
	}
}
void test_rect_within(Rect<int> r1, Rect<int> r2, bool expected)
{
	if (r1.within(r2) != expected)
	{
		LogError("Rect {%d,%d},{%d,%d} incorrectly %s rect {%d,%d},{%d,%d}", r2.p0.x, r2.p0.y,
		         r2.p1.x, r2.p1.y, expected ? "not within" : "within", r1.p0.x, r1.p0.y, r1.p1.x,
		         r1.p1.y);
		exit(EXIT_FAILURE);
	}
}
void test_rect_intersects(Rect<int> r1, Rect<int> r2, bool expected)
{
	if (r1.intersects(r2) != expected)
	{
		LogError("Rect {%d,%d},{%d,%d} incorrectly %s rect {%d,%d},{%d,%d}", r2.p0.x, r2.p0.y,
		         r2.p1.x, r2.p1.y, expected ? "does not intersect" : "intersects", r1.p0.x, r1.p0.y,
		         r1.p1.x, r1.p1.y);
		exit(EXIT_FAILURE);
	}
}
int main(int argc, char **argv)
{
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

	return EXIT_SUCCESS;
}
