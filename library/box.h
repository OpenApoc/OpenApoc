
#pragma once

#include "vector2.h"
#include "line.h"

class Box
{
	public:
		Vector2* TopLeft;
		Vector2* BottomRight;

		Box( int X, int Y, int Width, int Height );
		Box( Vector2* UpperLeft, int Width, int Height );
		Box( Vector2* UpperLeft, Vector2* LowerRight );
		~Box();

		float GetLeft();
		float GetRight();
		float GetTop();
		float GetBottom();
		float GetWidth();
		float GetHeight();

		bool Collides( Box* CheckAgainst );
};