
#pragma once

#include "../../framework/includes.h"

struct Colour
{
	unsigned char b;
	unsigned char g;
	unsigned char r;
	unsigned char a;
};

class Palette
{
	private:
		Colour* colours;

	public:
		Palette( std::string Filename );
		~Palette();

		Colour* GetColour(int Index);
		void SetColour(int Index, Colour* Col);
};

