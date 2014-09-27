#pragma once

#include "vec.h"

template <typename T>
class Rect
{
	private:
	public:
		Rect(T x0 = 0, T y0 = 0, T x1 = 0, T y1 = 0)
			: Rect(Vec2<T>(x0, y0), Vec2<T>(x1, y1)) {}
		Rect(Vec2<T> p0, Vec2<T> p1)
			: p0(p0), p1(p1) {}

		Vec2<T> p0, p1;

		bool intersects(Vec2<T> p){
			return (p.x >= p0.x &&
			        p.x <= p1.x &&
					p.y >= p0.y &&
					p.y <= p1.y);
		};
};
