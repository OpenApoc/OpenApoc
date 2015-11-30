#pragma once

#include "vec.h"
#include <set>

namespace OpenApoc
{

template <typename T> class Rect
{
  private:
  public:
	// Rects are inclusive of p0, exclusive of p1
	Rect(T x0 = 0, T y0 = 0, T x1 = 0, T y1 = 0) : Rect(Vec2<T>(x0, y0), Vec2<T>(x1, y1)) {}
	Rect(Vec2<T> p0, Vec2<T> p1) : p0(p0), p1(p1) {}

	Vec2<T> p0, p1;

	bool within(Vec2<T> p) const
	{
		return (p.x >= p0.x && p.x < p1.x && p.y >= p0.y && p.y < p1.y);
	}
	bool withinInclusive(Vec2<T> p) const
	{
		return (p.x >= p0.x && p.x <= p1.x && p.y >= p0.y && p.y <= p1.y);
	}
	bool within(Rect<T> r) const
	{
		return (r.p0.x >= p0.x && r.p1.x <= p1.x && r.p0.y >= p0.y && r.p1.y <= p1.y);
	}
	bool intersects(Rect<T> r) const
	{
		return !(r.p1.x <= p0.x || r.p0.x >= p1.x || r.p1.y <= p0.y || r.p0.y >= p1.y);
	}

	bool operator==(const Rect<T> &other) const { return p0 == other.p0 && p1 == other.p1; }

	Vec2<T> size() const { return Vec2<T>{p1.x - p0.x, p1.y - p0.y}; }

	// Something of an arbitrary order for storing in a set
	bool operator<(const Rect<T> &other) const
	{
		auto thisSize = this->size();
		auto otherSize = other.size();
		if (thisSize.x != otherSize.x)
			return thisSize.x < otherSize.x;
		if (thisSize.y != otherSize.y)
			return thisSize.y < otherSize.y;
		// If both height and width are the same they must have a different
		// p0, or be equal
		if (this->p0.x != other.p0.x)
			return this->p0.x < other.p0.x;
		return this->p0.y < other.p0.y;
	}

	// Attempts to merge rects adjacent rects in a set, returns the number merged.
	// This keeps looping until
	static int compactRectSet(std::set<Rect<T>> &rectSet)
	{
		int merged = 0;
	restart:
		for (auto it1 = rectSet.begin(); it1 != rectSet.end();)
		{
			auto rect1 = *it1++;
			for (auto it2 = rectSet.begin(); it2 != rectSet.end();)
			{
				auto rect2 = *it2++;
				if (rect1 == rect2)
					continue;
				auto rect1Size = rect1.size();
				auto rect2Size = rect2.size();
				bool canMerge = false;
				// Can merge rects where r1 lines up to r2 in x
				//
				//	+----+
				//	| r1 |
				//	+----+
				//	| r2 |
				//	+----+
				//
				//	or in y
				//	+----+----+
				//	| r1 | r2 |
				//	+----+----+
				//
				//	No need to check other way as r1 & r2 will be compared in both orders in the
				// loop
				//
				if (rect1Size.x == rect2Size.x && rect1.p0.x == rect2.p0.x &&
				    rect1.p1.y == rect2.p0.y)
				{
					canMerge = true;
				}
				if (rect1Size.y == rect2Size.y && rect1.p0.y == rect2.p0.y &&
				    rect1.p1.x == rect2.p0.x)
				{
					canMerge = true;
				}
				if (canMerge)
				{
					Rect<T> mergedRect{rect1.p0, rect2.p1};
					rectSet.erase(rect1);
					rectSet.erase(rect2);
					rectSet.insert(mergedRect);
					merged++;
					goto restart;
				}
			}
		}
		return merged;
	}
};
}; // namespace OpenApoc
