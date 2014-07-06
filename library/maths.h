
#pragma once

#include <cmath>

#ifndef M_PI
#define M_PI						3.141592654f
#endif

#define M_DEG_TO_RAD		0.01745329f		// (M_PI / 180.0f)
#define M_RAD_TO_DEG		57.2957795f		// (180.0f / M_PI )

class Maths
{
	public:
		//static float Pi() { return M_PI; };

		static int Min( int a, int b )
		{
			return ( a <= b ? a : b );
		};
		static float Min( float a, float b )
		{
			return ( a <= b ? a : b );
		};

		static int Max( int a, int b )
		{
			return ( a >= b ? a : b );
		};
		static float Max( float a, float b )
		{
			return ( a >= b ? a : b );
		};

		static int Abs( int a )
		{
			return ( a < 0 ? a * -1 : a );
		};
		static float Abs( float a )
		{
			return ( a < 0.0 ? a * -1 : a );
		};
};
