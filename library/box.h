
#pragma once

#include "vec.h"
#include "line.h"

namespace OpenApoc
{

template <typename T> class Box
{
  public:
	Vec2<T> TopLeft;
	Vec2<T> BottomRight;

	Box(T X, T Y, T Width, T Height);
	Box(const Vec2<T> UpperLeft, T Width, T Height);
	Box(const Vec2<T> UpperLeft, const Vec2<T> LowerRight);
	~Box();

	T GetLeft();
	T GetRight();
	T GetTop();
	T GetBottom();
	T GetWidth();
	T GetHeight();

	bool Collides(const Box<T> CheckAgainst);
};

template <typename T> class Cubeoid
{
  public:
	Vec3<T> p1;
	Vec3<T> p2;

	Cubeoid(T X, T Y, T Z, T W, T H, T D) : Cubeoid(Vec3<T>{X, Y, Z}, Vec3<T>{W, H, D}){};
	Cubeoid(const Vec3<T> origin, T W, T H, T D) : Cubeoid(origin, Vec3<T>{W, H, D}){};
	Cubeoid(const Vec3<T> p1, const Vec3<T> p2) : p1(p1), p2(p2){};
	~Cubeoid(){};

	bool Collides(const Cubeoid<T> CheckAgainst);
};

template <typename T> Box<T>::Box(T X, T Y, T Width, T Height)
{
	TopLeft = Vec2<T>(X, Y);
	BottomRight = Vec2<T>(TopLeft->X + Width, TopLeft->Y + Height);
}

template <typename T> Box<T>::Box(const Vec2<T> UpperLeft, T Width, T Height)
{
	TopLeft = Vec2<T>(UpperLeft);
	BottomRight = Vec2<T>(TopLeft->X + Width, TopLeft->Y + Height);
}

template <typename T> Box<T>::Box(const Vec2<T> UpperLeft, const Vec2<T> LowerRight)
{
	TopLeft = Vec2<T>(UpperLeft);
	BottomRight = Vec2<T>(LowerRight);
}

template <typename T> Box<T>::~Box() {}

template <typename T> T Box<T>::GetLeft() { return TopLeft->X; }

template <typename T> T Box<T>::GetRight() { return BottomRight->X; }

template <typename T> T Box<T>::GetTop() { return TopLeft->Y; }

template <typename T> T Box<T>::GetBottom() { return BottomRight->Y; }

template <typename T> T Box<T>::GetWidth() { return BottomRight->X - TopLeft->X; }

template <typename T> T Box<T>::GetHeight() { return BottomRight->Y - TopLeft->Y; }

template <typename T> bool Box<T>::Collides(const Box<T> CheckAgainst)
{
	return (GetLeft() <= CheckAgainst.GetRight() && GetRight() >= CheckAgainst.GetLeft() &&
	        GetTop() <= CheckAgainst.GetBottom() && GetBottom() >= CheckAgainst.GetTop());
}

} // namespace OpenApoc
