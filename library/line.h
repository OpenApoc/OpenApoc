
#pragma once

#include "vector2.h"
#include "angle.h"

class Line
{
	public:
		Vector2* Points[2];

		float XAdjust;
		float YAdjust;

		Line( Vector2* A, Vector2* B );
		Line( float x1, float y1, float x2, float y2 );
		~Line();

		float GetSlope();
		float GetIntercept();

		Vector2* GetIntersection( Line* IntersectsWith );
		Vector2* ToVector();
		Angle* ToAngle();

		Angle* Reflection( Line* Projection );

		Vector2* GetSegmentPoint( int SegmentNumber, int NumberOfSegments );
};