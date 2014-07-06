
#pragma once

#include "spritesheet.h"

class Animation
{

	private:
		SpriteSheet* sprites;
		std::vector<int> frameList;
		bool animRepeats;
		bool animHasEnded;
		int animCurrentFrame;
		int animDelayTime;
		int animDelayTick;
		float animScaleX;
		float animScaleY;

	public:
		Animation( SpriteSheet* Sprites, bool LoopAnimation, int FrameDuration );

		void AddFrame( int SpriteIndex );

		void Start();
		void Update();
		void DrawFrame( int ScreenX, int ScreenY );
		void DrawFrame( int ScreenX, int ScreenY, bool FlipX, bool FlipY );
		void DrawFrame( int ScreenX, int ScreenY, bool FlipX, bool FlipY, Angle* Rotation );
		bool Loops();
		bool HasEnded();
		int GetCurrentFrame();
		void SetScale( float NewScale );
		void SetScale( float X, float Y );
		float GetScaleX();
		float GetScaleY();


};