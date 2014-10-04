
#pragma once

#include "maths.h"

namespace OpenApoc {

class Vector2;

class Angle
{
	private:
		float curAngle;

	public:
		Angle();
		Angle( float Degrees );

		void Add( float Degrees );
		void Set( float Degrees );

		float ToDegrees();
		float ToRadians();
		Vector2* ToVector();

		float ShortestAngleTo( Angle* DestinationAngle );
		float ShortestAngleTo( float DestinationAngle );
		bool ClockwiseShortestTo( Angle* DestinationAngle );
		bool ClockwiseShortestTo( float DestinationAngle );
		void RotateShortestBy( Angle* DestinationAngle, float ByDegrees );
		void RotateShortestBy( float DestinationAngle, float ByDegrees );

		float Sine();
		float Cosine();
		float Tan();
};

}; //namespace OpenApoc
