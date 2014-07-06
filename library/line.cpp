
#include "line.h"
#include "maths.h"

Line::Line( Vector2* A, Vector2* B )
{
	XAdjust = 0;
	YAdjust = 0;
	Points[0] = new Vector2( A->X, A->Y );
	Points[1] = new Vector2( B->X, B->Y );
}

Line::Line( float x1, float y1, float x2, float y2 )
{
	XAdjust = 0;
	YAdjust = 0;
	Points[0] = new Vector2( x1, y1 );
	Points[1] = new Vector2( x2, y2 );
}

Line::~Line()
{
	delete Points[1];
	delete Points[0];
}

float Line::GetSlope()
{
	float slope;
	if( Points[0]->X == Points[1]->X )
	{
		slope = 999999999.9999f;
	} else {
		slope = (Points[1]->Y - Points[0]->Y) / (Points[1]->X - Points[0]->X);
	}
	return slope;
}

float Line::GetIntercept()
{
	return (Points[0]->Y + YAdjust) - (GetSlope() * (Points[0]->X + XAdjust));
}


Vector2* Line::GetIntersection( Line* IntersectsWith )
{
	float lx[2];
	float ly[2];
	float ix[2];
	float iy[2];
	float m[2];
	float b[2];

	lx[0] = Points[0]->X + XAdjust;
	lx[1] = Points[1]->X + XAdjust;
	ly[0] = Points[0]->Y + YAdjust;
	ly[1] = Points[1]->Y + YAdjust;
	ix[0] = IntersectsWith->Points[0]->X + IntersectsWith->XAdjust;
	ix[1] = IntersectsWith->Points[1]->X + IntersectsWith->XAdjust;
	iy[0] = IntersectsWith->Points[0]->Y + IntersectsWith->YAdjust;
	iy[1] = IntersectsWith->Points[1]->Y + IntersectsWith->YAdjust;

	m[0] = GetSlope();
	m[1] = IntersectsWith->GetSlope();
	b[0] = GetIntercept();
	b[1] = IntersectsWith->GetIntercept();

	float tmp;

	if( lx[0] == lx[1] )
	{
		if( Maths::Min(ix[0], ix[1]) <= lx[0]  && Maths::Max(ix[0], ix[1]) >= lx[0] )
		{
			tmp = (m[1] * lx[0]) + b[1];
			if( (tmp >= ly[0] && tmp <= ly[1]) || (tmp <= ly[0] && tmp >= ly[1]) )
			{
				return new Vector2( lx[0], tmp );
			}
		} else {
			return 0;
		}
	} else if ( ix[0] == ix[1] ) {
		if( Maths::Min(lx[0], lx[1]) <= ix[0]  && Maths::Max(lx[0], lx[1]) >= ix[0] )
		{
			tmp = (m[0] * ix[0]) + b[0];
			if( (tmp >= iy[0] && tmp <= iy[1]) || (tmp <= iy[0] && tmp >= iy[1]) )
			{
				return new Vector2( ix[0], tmp );
			}
		} else {
			return 0;
		}
	} else {
		tmp = (b[1] - b[0]) / (m[0] - m[1]);
		if( tmp >= (lx[0] <= lx[1] ? lx[0] : lx[1]) && (lx[0] >= lx[1] ? lx[0] : lx[1]) )
		{
			if( tmp >= (ix[0] <= ix[1] ? ix[0] : ix[1]) && (ix[0] >= ix[1] ? ix[0] : ix[1]) )
			{
				return new Vector2( tmp, (m[0] * tmp) + b[0] );
			}
		}
	}

	return 0;
}

Vector2* Line::ToVector()
{
	return new Vector2( Points[1]->X - Points[0]->X, Points[1]->Y - Points[0]->Y );
}

Angle* Line::ToAngle()
{
	return new Angle( Points[0]->AngleTo( Points[1] ) );
}

Angle* Line::Reflection( Line* Projection )
{
	// Lines don't intersect, no reflection
	Vector2* collision = GetIntersection( Projection );
	Angle* result = 0;

	if( collision == 0 )
	{
		return 0;
	}

	result = new Angle( collision->AngleTo( Projection->Points[0] ) );
	float angToLineL = collision->AngleTo( Points[0] ) + 90.0f;
	float angToLineR = collision->AngleTo( Points[0] ) - 90.0f;		// Do I need this?

	if( result->ShortestAngleTo( angToLineL ) < 90.0f )
	{
		// Closer to top edge
		if( result->ClockwiseShortestTo( angToLineL ) )
		{
			result->Add( result->ShortestAngleTo( angToLineL ) * 2.0f );
		} else {
			result->Add( result->ShortestAngleTo( angToLineL ) * -2.0f );
		}
	} else {
		// Closer to top edge
		if( result->ClockwiseShortestTo( angToLineR ) )
		{
			result->Add( result->ShortestAngleTo( angToLineR ) * 2.0f );
		} else {
			result->Add( result->ShortestAngleTo( angToLineR ) * -2.0f );
		}
	}

	delete collision;
	return result;
}

Vector2* Line::GetSegmentPoint( int SegmentNumber, int NumberOfSegments )
{
	float xSeg = (Points[1]->X - Points[0]->X) / NumberOfSegments;
	float ySeg = (Points[1]->Y - Points[0]->Y) / NumberOfSegments;

	return new Vector2( Points[0]->X + (xSeg * SegmentNumber), Points[0]->Y + (ySeg * SegmentNumber) );
}
