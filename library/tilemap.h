
#pragma once

#include "spritesheet.h"
#include "animation.h"

class TileMap
{

	private:
		SpriteSheet* tileGraphics;
		std::vector<Animation*> tileAnimations;
		int* tileData;
		int width;
		int height;
		int tileWidth;
		int tileHeight;

	public:
		TileMap( int MapWidth, int MapHeight );
		TileMap( SpriteSheet* Graphics, int MapWidth, int MapHeight );
		~TileMap();

		int AddAnimation( Animation* TileAnimation );

		int GetWidth();
		int GetHeight();
		int GetTileWidth();
		int GetTileHeight();

		int GetTile( int X, int Y );
		void SetTile( int X, int Y, int Value );	//  Use negative values for animated tiles (-1 = Anim 0)

		void Render( int OffsetX, int OffsetY );
		void Render( int OffsetX, int OffsetY, float Scale );
		void Render( int OffsetX, int OffsetY, float ScaleX, float ScaleY );

		Vector2* GetTiledPosition( int TileX, int TileY, int OffsetX, int OffsetY, float ScaleX, float ScaleY );

		void Update();
};
