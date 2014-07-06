
#include "tilemap.h"
#include "../framework/framework.h"
#include "../library/vector2.h"

TileMap::TileMap( int MapWidth, int MapHeight )
{
	tileData = (int*)malloc( MapWidth * MapHeight * sizeof(int) );
	memset( (void*)tileData, 0, MapWidth * MapHeight * sizeof(int) );
	width = MapWidth;
	height = MapHeight;

	tileGraphics = 0;
	tileWidth = 0;
	tileHeight = 0;
}

TileMap::TileMap( SpriteSheet* Graphics, int MapWidth, int MapHeight )
{
	tileGraphics = Graphics;
	tileData = (int*)malloc( MapWidth * MapHeight * sizeof(int) );
	memset( (void*)tileData, 0, MapWidth * MapHeight * sizeof(int) );
	width = MapWidth;
	height = MapHeight;

	if( tileGraphics != 0 )
	{
		SpriteSheetRegion* r = tileGraphics->GetFrame(0);
		tileWidth = r->Width;
		tileHeight = r->Height;
	} else {
		tileWidth = 0;
		tileHeight = 0;
	}
}

TileMap::~TileMap()
{
	free( (void*)tileData );
}

int TileMap::AddAnimation( Animation* TileAnimation )
{
	tileAnimations.push_back( TileAnimation );
	return tileAnimations.size() * -1;
}

int TileMap::GetWidth()
{
	return width;
}

int TileMap::GetHeight()
{
	return height;
}

int TileMap::GetTileWidth()
{
	return tileWidth;
}

int TileMap::GetTileHeight()
{
	return tileHeight;
}

int TileMap::GetTile( int X, int Y )
{
	if( X < 0 || Y < 0 || X >= width || Y >= height )
	{
		return 0;
	}

	return tileData[ (Y * width) + X ];
}

void TileMap::SetTile( int X, int Y, int Value )
{
	if( X < 0 || Y < 0 || X >= width || Y >= height )
	{
		return;
	}

	tileData[ (Y * width) + X ] = Value;
}

void TileMap::Render( int OffsetX, int OffsetY )
{
	Render( OffsetX, OffsetY, 1.0f, 1.0f );
}

void TileMap::Render( int OffsetX, int OffsetY, float Scale )
{
	Render( OffsetX, OffsetY, Scale, Scale );
}

void TileMap::Render( int OffsetX, int OffsetY, float ScaleX, float ScaleY )
{
	// Prevent non-graphical tilemaps from drawing
	if( tileGraphics == 0 )
	{
		return;
	}

	float scaledTileW = tileWidth * ScaleX;
	float scaledTileH = tileHeight * ScaleY;
	int startX = OffsetX / scaledTileW;
	int startY = OffsetY / scaledTileH;
	int endX = startX + (FRAMEWORK->Display_GetWidth() / scaledTileW) + 1;
	int endY = startY + (FRAMEWORK->Display_GetHeight() / scaledTileH) + 1;

	al_hold_bitmap_drawing( true );

	for( int ty = startY; ty <= endY; ty++ )
	{
		if( ty >= height )
		{
			break;
		}
		int actualy = (ty * scaledTileH) - OffsetY;

		for( int tx = startX; tx <= endX; tx++ )
		{
			if( tx >= width )
			{
				break;
			}
			int actualx = (tx * scaledTileW) - OffsetX;
			int tileIdx = tileData[ (ty * width) + tx ];

			if( tileIdx < 0 )
			{
				// Draw Animated Tile
				int animidx = (tileIdx * -1) - 1;
				if( animidx < (int)tileAnimations.size() )
				{
					Animation* a = tileAnimations.at( animidx );
					a->SetScale( ScaleX, ScaleY );
					a->DrawFrame( actualx, actualy );
				}
			} else {
				// Draw Normal Tile
				tileGraphics->DrawSprite( tileIdx, actualx, actualy, ScaleX, ScaleY, 0 );
			}

		}
	}

	al_hold_bitmap_drawing( false );
}

Vector2* TileMap::GetTiledPosition( int TileX, int TileY, int OffsetX, int OffsetY, float ScaleX, float ScaleY )
{
	return new Vector2( (TileX * (tileWidth * ScaleX)) - OffsetX, (TileY * (tileHeight * ScaleY)) - OffsetY );
}

void TileMap::Update()
{
	if( tileAnimations.size() > 0 )
	{
		for( std::vector<Animation*>::const_iterator i = tileAnimations.begin(); i != tileAnimations.end(); i++ )
		{
			Animation* a = (Animation*)(*i);
			a->Update();
		}
	}
}
