
#ifndef TILE_CONVERTER_H
#define TILE_CONVERTER_H

extern struct tCache{

    byte data[3][0x40000];
    byte valid[3][0x1000];
    
} TileCache;

#define H_FLIP 0x4000
#define V_FLIP 0x8000
#define BLANK_TILE	2

extern void DrawTile ( dword Tile, int x ,int y, int StartPixel, int Width,int StartLine,int LineCount);

extern void resetTileConverter(void);

#endif
