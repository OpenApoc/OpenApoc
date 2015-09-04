
#pragma once

#include "vec.h"
#include "angle.h"

namespace OpenApoc
{

template <typename T> class Line
{
  private:
	Vec2<T> Points[2];

	T XAdjust;
	T YAdjust;

  public:
	Line(const Vec2<T> A, const Vec2<T> B);
	Line(T x1, T y1, T x2, T y2);
	~Line();

	T GetSlope();
	T GetIntercept();

	Vec2<T> GetIntersection(const Line<T> IntersectsWith);
	Vec2<T> ToVector();
	Angle<T> ToAngle();

	Angle<T> Reflection(const Line<T> Projection);

	Vec2<T> GetSegmentPoint(int SegmentNumber, int NumberOfSegments);
};

template <typename T> Line<T>::Line(const Vec2<T> A, const Vec2<T> B)
{
	XAdjust = 0;
	YAdjust = 0;
	Points[0] = A;
	Points[1] = B;
}

template <typename T> Line<T>::Line(T x1, T y1, T x2, T y2)
{
	XAdjust = 0;
	YAdjust = 0;
	Points[0] = Vec2<T>(x1, y1);
	Points[1] = Vec2<T>(x2, y2);
}

template <typename T> Line<T>::~Line() {}

template <typename T> T Line<T>::GetSlope()
{
	T slope;
	if (Points[0].X == Points[1].X) {
		slope = std::numeric_limits<T>::max();
	} else {
		slope = (Points[1].Y - Points[0].Y) / (Points[1].X - Points[0].X);
	}
	return slope;
}

template <typename T> T Line<T>::GetIntercept()
{
	return (Points[0].Y + YAdjust) - (GetSlope() * (Points[0].X + XAdjust));
}

template <typename T> Vec2<T> Line<T>::GetIntersection(const Line<T> IntersectsWith)
{
	T lx[2];
	T ly[2];
	T ix[2];
	T iy[2];
	T m[2];
	T b[2];

	lx[0] = Points[0].X + XAdjust;
	lx[1] = Points[1].X + XAdjust;
	ly[0] = Points[0].Y + YAdjust;
	ly[1] = Points[1].Y + YAdjust;
	ix[0] = IntersectsWith.Points[0].X + IntersectsWith.XAdjust;
	ix[1] = IntersectsWith.Points[1].X + IntersectsWith.XAdjust;
	iy[0] = IntersectsWith.Points[0].Y + IntersectsWith.YAdjust;
	iy[1] = IntersectsWith.Points[1].Y + IntersectsWith.YAdjust;

	m[0] = GetSlope();
	m[1] = IntersectsWith.GetSlope();
	b[0] = GetIntercept();
	b[1] = IntersectsWith.GetIntercept();

	T tmp;

	if (lx[0] == lx[1]) {
		if (Maths::Min(ix[0], ix[1]) <= lx[0] && Maths::Max(ix[0], ix[1]) >= lx[0]) {
			tmp = (m[1] * lx[0]) + b[1];
			if ((tmp >= ly[0] && tmp <= ly[1]) || (tmp <= ly[0] && tmp >= ly[1])) {
				return new Vec2<T>(lx[0], tmp);
			}
		} else {
			return 0;
		}
	} else if (ix[0] == ix[1]) {
		if (Maths::Min(lx[0], lx[1]) <= ix[0] && Maths::Max(lx[0], lx[1]) >= ix[0]) {
			tmp = (m[0] * ix[0]) + b[0];
			if ((tmp >= iy[0] && tmp <= iy[1]) || (tmp <= iy[0] && tmp >= iy[1])) {
				return new Vec2<T>(ix[0], tmp);
			}
		} else {
			return 0;
		}
	} else {
		tmp = (b[1] - b[0]) / (m[0] - m[1]);
		if (tmp >= (lx[0] <= lx[1] ? lx[0] : lx[1]) && (lx[0] >= lx[1] ? lx[0] : lx[1])) {
			if (tmp >= (ix[0] <= ix[1] ? ix[0] : ix[1]) && (ix[0] >= ix[1] ? ix[0] : ix[1])) {
				return new Vec2<T>(tmp, (m[0] * tmp) + b[0]);
			}
		}
	}

	return 0;
}

template <typename T> Vec2<T> Line<T>::ToVector()
{
	return new Vec2<T>(Points[1].X - Points[0].X, Points[1].Y - Points[0].Y);
}

template <typename T> Angle<T> Line<T>::ToAngle() { return Angle<T>(Points[0].AngleTo(Points[1])); }

template <typename T> Angle<T> Line<T>::Reflection(const Line<T> Projection)
{
	// Lines don't intersect, no reflection
	Vec2<T> collision = GetIntersection(Projection);
	Angle<T> result = 0;

	if (collision == 0) {
		return 0;
	}

	result = Angle<T>(collision.AngleTo(Projection.Points[0]));
	T angToLineL = collision.AngleTo(Points[0]) + 90.0f;
	T angToLineR = collision.AngleTo(Points[0]) - 90.0f; // Do I need this?

	if (result.ShortestAngleTo(angToLineL) < 90.0f) {
		// Closer to top edge
		if (result.ClockwiseShortestTo(angToLineL)) {
			result.Add(result.ShortestAngleTo(angToLineL) * 2.0f);
		} else {
			result.Add(result.ShortestAngleTo(angToLineL) * -2.0f);
		}
	} else {
		// Closer to top edge
		if (result.ClockwiseShortestTo(angToLineR)) {
			result.Add(result.ShortestAngleTo(angToLineR) * 2.0f);
		} else {
			result.Add(result.ShortestAngleTo(angToLineR) * -2.0f);
		}
	}

	return result;
}

template <typename T> Vec2<T> Line<T>::GetSegmentPoint(int SegmentNumber, int NumberOfSegments)
{
	T xSeg = (Points[1].X - Points[0].X) / NumberOfSegments;
	T ySeg = (Points[1].Y - Points[0].Y) / NumberOfSegments;

	return new Vec2<T>(Points[0].X + (xSeg * SegmentNumber), Points[0].Y + (ySeg * SegmentNumber));
}

} // namespace OpenApoc
