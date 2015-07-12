#pragma once

#include "vec.h"

namespace OpenApoc {

template <typename T>
class Rect
{
	private:
	public:
		//Rects are inclusive of p0, exclusive of p1
		Rect(T x0 = 0, T y0 = 0, T x1 = 0, T y1 = 0)
			: Rect(Vec2<T>(x0, y0), Vec2<T>(x1, y1)) {}
		Rect(Vec2<T> p0, Vec2<T> p1)
			: p0(p0), p1(p1) {}

		Vec2<T> p0, p1;

		bool within(Vec2<T> p) const {
			return (p.x >= p0.x &&
			        p.x < p1.x &&
			        p.y >= p0.y &&
			        p.y < p1.y);
		};
		bool withinInclusive(Vec2<T> p) const {
			return (p.x >= p0.x &&
					p.x <= p1.x &&
					p.y >= p0.y &&
					p.y <= p1.y);
		};
		bool within(Rect<T> r) const {
			return (r.p0.x >= p0.x &&
			        r.p1.x <= p1.x &&
			        r.p0.y >= p0.y &&
			        r.p1.y <= p1.y);
		};
		bool intersects(Rect<T> r) const {
			return !(r.p1.x <= p0.x ||
			         r.p0.x >= p1.x ||
			         r.p1.y <= p0.y ||
			         r.p0.y >= p1.y);
		};

		bool operator==(const Rect<T> &other) const
		{
			return p0 == other.p0 && p1 == other.p1;
		}

		Vec2<T> size() const
		{
			return Vec2<T>{p1.x-p0.x, p1.y-p0.y};
		}
};
}; //namespace OpenApoc
