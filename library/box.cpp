
#include "box.h"

Box::Box( int X, int Y, int Width, int Height )
{
	TopLeft = new Vector2( X, Y );
	BottomRight = new Vector2( TopLeft->X + Width, TopLeft->Y + Height );
}

Box::Box( Vector2* UpperLeft, int Width, int Height )
{
	TopLeft = new Vector2( UpperLeft );
	BottomRight = new Vector2( TopLeft->X + Width, TopLeft->Y + Height );
}

Box::Box( Vector2* UpperLeft, Vector2* LowerRight )
{
	TopLeft = new Vector2( UpperLeft );
	BottomRight = new Vector2( LowerRight );
}

Box::~Box()
{
	delete TopLeft;
	delete BottomRight;
}

float Box::GetLeft()
{
	return TopLeft->X;
}

float Box::GetRight()
{
	return BottomRight->X;
}

float Box::GetTop()
{
	return TopLeft->Y;
}

float Box::GetBottom()
{
	return BottomRight->Y;
}

float Box::GetWidth()
{
	return BottomRight->X - TopLeft->X;
}

float Box::GetHeight()
{
	return BottomRight->Y - TopLeft->Y;
}

bool Box::Collides( Box* CheckAgainst )
{
	return ( GetLeft() <= CheckAgainst->GetRight() && GetRight() >= CheckAgainst->GetLeft() &&
			GetTop() <= CheckAgainst->GetBottom() && GetBottom() >= CheckAgainst->GetTop() );
}