
#pragma once

#include "maths.h"

#ifndef Angle
class Angle;
#endif

class Vector2
{
	public:
		float X;
		float Y;

		Vector2();
		Vector2( float X, float Y );
		Vector2( Vector2* Copy );
		Vector2( float Degrees );
		Vector2( Angle* Direction );

		void RotateVector( float Degrees );
		void RotateVector( Angle* Direction );
		void RotateVector( float Degrees, Vector2* RotationOrigin );
		void RotateVector( Angle* Direction, Vector2* RotationOrigin );

		Vector2 operator+=(Vector2 A);

		bool operator==(Vector2 A);
		bool operator!=(Vector2 A);

		void Add(Vector2* Point);
		void Subtract(Vector2* Point);
		void Multiply(float Multiplier);
		float AngleTo( Vector2* CheckPoint );
		float DistanceTo( Vector2* CheckPoint );
		float DotProduct( Vector2* a );
		void Normalise();

		Angle* ToAngle();
};

