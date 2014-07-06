
#include "vector2.h"
#include "angle.h"

Vector2::Vector2()
{
	X = 0;
	Y = 0;
}

Vector2::Vector2( float X, float Y )
{
	this->X = X;
	this->Y = Y;
}

Vector2::Vector2( Vector2* Copy )
{
	X = Copy->X;
	Y = Copy->Y;
}

Vector2::Vector2( float Degrees )
{
	X = sin(Degrees * M_DEG_TO_RAD);
	Y = -cos(Degrees * M_DEG_TO_RAD);
}

Vector2::Vector2( Angle* Direction )
{
	X = Direction->Sine();
	Y = -Direction->Cosine();
}

void Vector2::RotateVector( float Degrees )
{
	Vector2* tmp = new Vector2( 0, 0 );
	Angle* a = new Angle( Degrees );
	RotateVector( a, tmp );
	delete tmp;
	delete a;
}

void Vector2::RotateVector( Angle* Direction )
{
  Vector2* tmp = new Vector2( 0, 0 );
  RotateVector( Direction, tmp );
  delete tmp;
}

void Vector2::RotateVector( float Degrees, Vector2* RotationOrigin )
{
	Angle* a = new Angle( Degrees );
	return RotateVector( a, RotationOrigin );
	delete a;
}
void Vector2::RotateVector( Angle* Direction, Vector2* RotationOrigin )
{
	float rotSin = Direction->Sine();
	float rotCos = Direction->Cosine();
	float tmpX;
	tmpX = ((X - RotationOrigin->X) * rotCos) - ((Y - RotationOrigin->Y) * rotSin) + RotationOrigin->X;
	Y = ((Y - RotationOrigin->Y) * rotCos) + ((X - RotationOrigin->X) * rotSin) + RotationOrigin->Y;
	X = tmpX;
}

Vector2 Vector2::operator+=(Vector2 A)
{
	X += A.X;
	Y += A.Y;
	return *this;
}

bool Vector2::operator==( Vector2 A )
{
	return (this->X == A.X && this->Y == A.Y);
}

bool Vector2::operator!=( Vector2 A )
{
	return (this->X != A.X || this->Y != A.Y);
}

void Vector2::Add(Vector2* Point)
{
	X += Point->X;
	Y += Point->Y;
}

void Vector2::Subtract(Vector2* Point)
{
	X -= Point->X;
	Y -= Point->Y;
}

void Vector2::Multiply(float Multiplier)
{
	X *= Multiplier;
	Y *= Multiplier;
}

float Vector2::AngleTo( Vector2* CheckPoint )
{
	float r = atan2( CheckPoint->X - X, Y - CheckPoint->Y ) * M_RAD_TO_DEG;
	while( r >= 360.0f )
	{
		r -= 360.0f;
	}
	while( r < 0.0f )
	{
		r += 360.0f;
	}
	return r;
}

float Vector2::DistanceTo( Vector2* CheckPoint )
{
	return sqrt( pow(CheckPoint->X - X, 2.0f) + pow(CheckPoint->Y - Y, 2.0f) );
}

float Vector2::DotProduct( Vector2* a )
{
	return (X * a->X) + (Y * a->Y);
}

void Vector2::Normalise()
{
	float mag = sqrt( (X*X) + (Y*Y) );
	X /= mag;
	Y /= mag;
}

Angle* Vector2::ToAngle()
{
	float r = atan2( X, -Y ) * M_RAD_TO_DEG;
	return new Angle( r );
}
