
#include "animation.h"

Animation::Animation( SpriteSheet* Sprites, bool LoopAnimation, int FrameDuration )
{
	sprites = Sprites;
	animRepeats = LoopAnimation;
	animDelayTime = FrameDuration;
	animScaleX = 1.0;
	animScaleY = 1.0;
	Start();
}

void Animation::AddFrame( int SpriteIndex )
{
	frameList.push_back( SpriteIndex );
}

void Animation::Start()
{
	animDelayTick = 0;
	animCurrentFrame = 0;
	animHasEnded = false;
}

void Animation::Update()
{
	animDelayTick = (animDelayTick + 1) % animDelayTime;
	if( animDelayTick == 0 )
	{
		animCurrentFrame = (animCurrentFrame + 1) % frameList.size();
		if( animCurrentFrame == 0 && !animRepeats )
		{
			animHasEnded = true;
		}
	}
}

void Animation::DrawFrame( int ScreenX, int ScreenY )
{
	sprites->DrawSprite( frameList.at(animCurrentFrame), ScreenX, ScreenY, animScaleX, animScaleY, 0 );
}

void Animation::DrawFrame( int ScreenX, int ScreenY, bool FlipX, bool FlipY )
{
	sprites->DrawSprite( frameList.at(animCurrentFrame), ScreenX, ScreenY, animScaleX * (FlipX ? -1 : 1), animScaleY * (FlipY ? -1 : 1), 0 );
}

void Animation::DrawFrame( int ScreenX, int ScreenY, bool FlipX, bool FlipY, Angle* Rotation )
{
	sprites->DrawSprite( frameList.at(animCurrentFrame), ScreenX, ScreenY, animScaleX * (FlipX ? -1 : 1), animScaleY * (FlipY ? -1 : 1), Rotation );
}

bool Animation::Loops()
{
	return animRepeats;
}

bool Animation::HasEnded()
{
	return animHasEnded;
}

int Animation::GetCurrentFrame()
{
	return animCurrentFrame;
}

void Animation::SetScale( float NewScale )
{
	animScaleX = NewScale;
	animScaleY = NewScale;
}

void Animation::SetScale( float X, float Y )
{
	animScaleX = X;
	animScaleY = Y;
}

float Animation::GetScaleX()
{
	return animScaleX;
}

float Animation::GetScaleY()
{
	return animScaleY;
}
