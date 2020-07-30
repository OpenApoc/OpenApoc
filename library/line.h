#pragma once

#include "library/vec.h"
#include <glm/glm.hpp>
#include <iterator>

namespace OpenApoc
{

template <typename T, bool conservative> class LineSegmentIterator;

template <typename T, bool conservative> class LineSegment
{
  public:
	Vec3<T> startPoint;
	Vec3<T> endPoint;
	Vec3<T> inc;
	T increment;
	LineSegment(Vec3<T> start, Vec3<T> end, T increment = 1)
	    : startPoint(start), endPoint(end), increment(increment)
	{
		Vec3<T> d = endPoint - startPoint;
		inc.x = (d.x < static_cast<T>(0)) ? -increment : increment;
		inc.y = (d.y < static_cast<T>(0)) ? -increment : increment;
		inc.z = (d.z < static_cast<T>(0)) ? -increment : increment;
	}
	LineSegmentIterator<T, conservative> begin() const;
	LineSegmentIterator<T, conservative> end() const;
};

template <typename T, bool conservative> class LineSegmentIterator
{
  private:
	Vec3<T> point;
	Vec3<T> err;
	Vec3<T> step;
	Vec3<T> d2;
	Vec3<T> inc;
	T dstep2;

	const LineSegment<T, conservative> &line;

  public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = Vec3<T>;
	using difference_type = ptrdiff_t;
	using pointer = Vec3<T> *;
	using reference = Vec3<T> &;
	LineSegmentIterator(Vec3<T> start, const LineSegment<T, conservative> &l)
	    : point(start), line(l)
	{
		err = {static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)};
		step = {static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)};
		Vec3<T> d = l.endPoint - l.startPoint;
		inc = l.inc;
		Vec3<T> absd = glm::abs(d);
		d2 = absd * static_cast<T>(2);
		if (absd.x >= absd.y && absd.x >= absd.z)
		{
			step.x = inc.x;
			dstep2 = d2.x;
			d2.x = static_cast<T>(0);
		}
		else if (absd.y >= absd.x && absd.y >= absd.z)
		{
			step.y = inc.y;
			dstep2 = d2.y;
			d2.y = static_cast<T>(0);
		}
		else if (absd.z >= absd.x && absd.z >= absd.y)
		{
			step.z = inc.z;
			dstep2 = d2.z;
			d2.z = static_cast<T>(0);
		}
	}

	LineSegmentIterator &operator++()
	{
		if (conservative)
		{
			if (err.x > static_cast<T>(0))
			{
				point.x += inc.x;
				err.x -= dstep2;
			}
			else if (err.y > static_cast<T>(0))
			{
				point.y += inc.y;
				err.y -= dstep2;
			}
			else if (err.z > static_cast<T>(0))
			{
				point.z += inc.z;
				err.z -= dstep2;
			}
			else
			{
				err += d2;
				point += step;
			}
		}
		else
		{
			if (err.x > static_cast<T>(0))
			{
				point.x += inc.x;
				err.x -= dstep2;
			}
			if (err.y > static_cast<T>(0))
			{
				point.y += inc.y;
				err.y -= dstep2;
			}
			if (err.z > static_cast<T>(0))
			{
				point.z += inc.z;
				err.z -= dstep2;
			}
			err += d2;
			point += step;
		}
		return *this;
	}

	bool operator==(const LineSegmentIterator &other) const
	{
		return (this->point * step == other.point * step);
	}

	bool operator!=(const LineSegmentIterator &other) const { return !(*this == other); }

	Vec3<T> &operator*() { return point; }
};

template <typename T, bool conservative>
LineSegmentIterator<T, conservative> LineSegment<T, conservative>::begin() const
{
	return LineSegmentIterator<T, conservative>(this->startPoint, *this);
}

template <typename T, bool conservative>
LineSegmentIterator<T, conservative> LineSegment<T, conservative>::end() const
{
	return LineSegmentIterator<T, conservative>(this->endPoint + this->inc, *this);
}

} // namespace OpenApoc
