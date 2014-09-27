#pragma once

template <typename T>
class Vec2
{
	private:
	public:
		Vec2(T x = 0, T y = 0)
			: x(x), y(y) {}
		T x, y;
};

template <typename T>
class Vec3
{
	private:
	public:
		Vec3(T x = 0, T y = 0, T z = 0)
			: x(x), y(y), z(z) {}
		T x, y, z;
};
