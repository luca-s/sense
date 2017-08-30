              
/*
 This functions draw the tile passed as argument in the screen
*/
#define SET_PIX( p, x, casting_type, value)\
	if(BG.currentTileDepth > z_buffer.depth[Y+y][X+x])\
	{\
		if(value)\
		{\
			*(casting_type *)(p + (x * SCREEN.bpp) )  =  tilePalette[value];\
			z_buffer.depth[Y+y][X+x] = BG.currentTileDepth;\
		}\
	}

 		                                                                    
#define RENDER_TILE(casting_type)\
 int y;\
 Uint8 *p = (Uint8 *)DISPLAY.screen->pixels;\
 Uint32* tilePalette = &convertedPalette[paletteOffset];\
 data+= StartLine+StartPixel;\
              p += (Y * SCREEN.bpl) + (X * SCREEN.bpp);\
             	for (y = 0; y < LineCount; y++)\
	{\
		switch(Width)\
		{\
			case 8:\
                			SET_PIX( p, 7,casting_type, *(data+7));\
                		case 7:\
                			SET_PIX( p, 6,casting_type, *(data+6));\
                		case 6:\
                			SET_PIX( p, 5,casting_type, *(data+5));\
                		case 5:\
                			SET_PIX( p, 4,casting_type, *(data+4));\
                		case 4:\
                			SET_PIX( p, 3,casting_type, *(data+3));\
                		case 3:\
                			SET_PIX( p, 2,casting_type, *(data+2));\
                		case 2:\
                			SET_PIX( p, 1,casting_type, *(data+1));\
                		case 1:\
                			SET_PIX( p, 0,casting_type, *(data+0));\
		}\
		data +=8;\
		p += SCREEN.bpl;\
	}\

#define RENDER_TILE_VFLIP(casting_type)\
 int y;\
 Uint8 *p = (Uint8 *)DISPLAY.screen->pixels;\
 Uint32* tilePalette = &convertedPalette[paletteOffset];\
 data+= 56 - StartLine+StartPixel;\
              p += (Y * SCREEN.bpl) + (X * SCREEN.bpp);\
             	for (y = 0; y < LineCount; y++)\
	{\
		switch(Width)\
		{\
			case 8:\
                			SET_PIX( p, 7,casting_type, *(data+7));\
                		case 7:\
                			SET_PIX( p, 6,casting_type, *(data+6));\
                		case 6:\
                			SET_PIX( p, 5,casting_type, *(data+5));\
                		case 5:\
                			SET_PIX( p, 4,casting_type, *(data+4));\
                		case 4:\
                			SET_PIX( p, 3,casting_type, *(data+3));\
                		case 3:\
                			SET_PIX( p, 2,casting_type, *(data+2));\
                		case 2:\
                			SET_PIX( p, 1,casting_type, *(data+1));\
                		case 1:\
                			SET_PIX( p, 0,casting_type, *(data+0));\
		}\
		data -=8;\
		p += SCREEN.bpl;\
	}\
	
	
#define RENDER_TILE_HFLIP(casting_type)\
 int y;\
 Uint8 *p = (Uint8 *)DISPLAY.screen->pixels;\
 Uint32* tilePalette = &convertedPalette[paletteOffset];\
 data+= StartLine+ 7 - StartPixel;\
              p += (Y * SCREEN.bpl) + (X * SCREEN.bpp);\
             	for (y = 0; y < LineCount; y++)\
	{\
		switch(Width)\
		{\
			case 8:\
                			SET_PIX( p, 7,casting_type, *(data-7));\
                		case 7:\
                			SET_PIX( p, 6,casting_type, *(data-6));\
                		case 6:\
                			SET_PIX( p, 5,casting_type, *(data-5));\
                		case 5:\
                			SET_PIX( p, 4,casting_type, *(data-4));\
                		case 4:\
                			SET_PIX( p, 3,casting_type, *(data-3));\
                		case 3:\
                			SET_PIX( p, 2,casting_type, *(data-2));\
                		case 2:\
                			SET_PIX( p, 1,casting_type, *(data-1));\
                		case 1:\
                			SET_PIX( p, 0,casting_type, *(data-0));\
		}\
		data +=8;\
		p += SCREEN.bpl;\
	}\
			
#define RENDER_TILE_HVFLIP(casting_type)\
 int y;\
 Uint8 *p = (Uint8 *)DISPLAY.screen->pixels;\
 Uint32* tilePalette = &convertedPalette[paletteOffset];\
 data+= 56 - StartLine + 7 - StartPixel;\
              p += (Y * SCREEN.bpl) + (X * SCREEN.bpp);\
             	for (y = 0; y < LineCount; y++)\
	{\
		switch(Width)\
		{\
			case 8:\
                			SET_PIX( p, 7,casting_type, *(data-7));\
                		case 7:\
                			SET_PIX( p, 6,casting_type, *(data-6));\
                		case 6:\
                			SET_PIX( p, 5,casting_type, *(data-5));\
                		case 5:\
                			SET_PIX( p, 4,casting_type, *(data-4));\
                		case 4:\
                			SET_PIX( p, 3,casting_type, *(data-3));\
                		case 3:\
                			SET_PIX( p, 2,casting_type, *(data-2));\
                		case 2:\
                			SET_PIX( p, 1,casting_type, *(data-1));\
                		case 1:\
                			SET_PIX( p, 0,casting_type, *(data-0));\
		}\
		data -=8;\
		p += SCREEN.bpl;\
	}\



/*****************1byte screen color depth************************/
void renderTile1(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE(Uint8)
}

void renderTile1Vflip(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_VFLIP(Uint8)
}

void renderTile1Hflip(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HFLIP(Uint8)
}

void renderTile1HVflip(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HVFLIP(Uint8)
}


/*****************2byte color depth************************/
		
void renderTile2(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE(Uint16)
}

void renderTile2Vflip(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_VFLIP(Uint16)
}

void renderTile2Hflip(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HFLIP(Uint16)
}

void renderTile2HVflip(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HVFLIP(Uint16)
}

/*****************4 byte color depth************************/

void renderTile4(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE(Uint32)
}

void renderTile4Vflip(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_VFLIP(Uint32)
}

void renderTile4Hflip(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HFLIP(Uint32)
}

void renderTile4HVflip(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HVFLIP(Uint32)
}

#undef SET_PIX
#define SET_PIX( p, x, casting_type, value)\
	if(BG.currentTileDepth > z_buffer.depth[Y+y][X+x])\
	{\
		if(value)\
		{\
			Uint32 c = tilePalette[value];\
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {\
				((casting_type *)(p + (x * SCREEN.bpp) ))[0] = (c >> 16) & 0xff;\
				((casting_type *)(p + (x * SCREEN.bpp) ))[1] = (c >> 8) & 0xff;\
				((casting_type *)(p + (x * SCREEN.bpp) ))[2] = c & 0xff;\
			}\
			else\
			{\
				((casting_type *)(p + (x * SCREEN.bpp) ))[2] = ( c >> 16) & 0xff;\
				((casting_type *)(p + (x * SCREEN.bpp) ))[1] = ( c >> 8) & 0xff;\
				((casting_type *)(p + (x * SCREEN.bpp) ))[0] = c & 0xff;\
			}\
			z_buffer.depth[Y+y][X+x] = BG.currentTileDepth;\
		}\
	}
	
/*****************3byte color depth************************/
void renderTile3(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE(Uint8)
}

void renderTile3Vflip(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{                       
 RENDER_TILE_VFLIP(Uint8)
}

void renderTile3Hflip(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HFLIP(Uint8)
}

void renderTile3HVflip(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HVFLIP(Uint8)
}


#define putpixel(offset,x,y,pixel)	(*pputpixel)(x,y,pixel)

extern void (*pputpixel)(int x, int y, byte pixel);

void (*pputpixel)(int x, int y, byte pixel);

void putpixel1(int x, int y, byte pixel)
{
    // Here p is the address to the pixel we want to set 
    Uint8 *p= (Uint8 *)DISPLAY.pixels + (y * SCREEN.bpl) + (x * SCREEN.bpp);
    Uint32 color =   getColor(pixel);
        
  *(Uint8*)p = color;
 
}


void putpixel2(int x, int y, byte pixel)
{
    // Here p is the address to the pixel we want to set 
    Uint8 *p= (Uint8 *)DISPLAY.pixels + (y * SCREEN.bpl) + (x * SCREEN.bpp);
    Uint32 color =   getColor(pixel);
        
  *(Uint16*)p = color;
 
}

void putpixel3(int x, int y, byte pixel)
{
    // Here p is the address to the pixel we want to set 
    Uint8 *p= (Uint8 *)DISPLAY.pixels + (y * SCREEN.bpl) + (x * SCREEN.bpp);
    Uint32 color =   getColor(pixel);
            
    if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	           p[0] = (color >> 16) & 0xff;
	           p[1] = (color >> 8) & 0xff;
	           p[2] = color & 0xff;
    } else {
	            p[0] = color & 0xff;
	            p[1] = (color >> 8) & 0xff;
	            p[2] = (color >> 16) & 0xff;
   }
       
}

void putpixel4(int x, int y, byte pixel)
{
    // Here p is the address to the pixel we want to set 
    Uint8 *p= (Uint8 *)DISPLAY.pixels + (y * SCREEN.bpl) + (x * SCREEN.bpp);
    Uint32 color =  getColor(pixel);
    
    *(Uint32 *)p = color;
    
}

                      
#undef SET_PIX
#define SET_PIX( p, x, casting_type, value)\
	if(BG.currentTileDepth > z_buffer.depth[Y+y][X+x])\
	{\
		if(value)\
		{\
			*(casting_type *)(p + (x * SCREEN.bpp) )  =\
			*(casting_type *)(p + (x * SCREEN.bpp) + DISPLAY.screen->format->BytesPerPixel)  =\
			*(casting_type *)(p + (x * SCREEN.bpp) + DISPLAY.screen->pitch)  =\
			*(casting_type *)(p + (x * SCREEN.bpp) + DISPLAY.screen->format->BytesPerPixel + DISPLAY.screen->pitch)  =\
			 			tilePalette[value];\
			z_buffer.depth[Y+y][X+x] = BG.currentTileDepth;\
		}\
	}


/*****************1byte screen color depth************************/
void renderTile1x2(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE(Uint8)
}

void renderTile1Vflipx2(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_VFLIP(Uint8)
}

void renderTile1Hflipx2(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HFLIP(Uint8)
}

void renderTile1HVflipx2(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HVFLIP(Uint8)
}


/*****************2byte color depth************************/
		
void renderTile2x2(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE(Uint16)
}

void renderTile2Vflipx2(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_VFLIP(Uint16)
}

void renderTile2Hflipx2(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HFLIP(Uint16)
}

void renderTile2HVflipx2(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HVFLIP(Uint16)
}

/*****************4 byte color depth************************/

void renderTile4x2(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE(Uint32)
}

void renderTile4Vflipx2(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_VFLIP(Uint32)
}

void renderTile4Hflipx2(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HFLIP(Uint32)
}

void renderTile4HVflipx2(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HVFLIP(Uint32)
}
                                                                                                           /*
#undef SET_PIX
#define SET_PIX( p, x, casting_type, value)\
	if(BG.currentTileDepth > z_buffer.depth[Y+y][X+x])\
	{\
		if(value)\
		{\
			Uint32 c = tilePalette[value];\
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {\
				((casting_type *)(p + (x * SCREEN.bpp) ))[0] = (c >> 16) & 0xff;\
				((casting_type *)(p + (x * SCREEN.bpp) ))[1] = (c >> 8) & 0xff;\
				((casting_type *)(p + (x * SCREEN.bpp) ))[2] = c & 0xff;\
			}\
			else\
			{\
				((casting_type *)(p + (x * SCREEN.bpp) ))[2] = ( c >> 16) & 0xff;\
				((casting_type *)(p + (x * SCREEN.bpp) ))[1] = ( c >> 8) & 0xff;\
				((casting_type *)(p + (x * SCREEN.bpp) ))[0] = c & 0xff;\
			}\
			z_buffer.depth[Y+y][X+x] = BG.currentTileDepth;\
		}\
	}
	                                                                                          */
/*****************3byte color depth************************//*
void renderTile3(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE(Uint8)
}

void renderTile3Vflip(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{                       
 RENDER_TILE_VFLIP(Uint8)
}

void renderTile3Hflip(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HFLIP(Uint8)
}

void renderTile3HVflip(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount)
{
 RENDER_TILE_HVFLIP(Uint8)
}
              */

              /*this function map the snes palette with the a palette suitable for screen display*/
void UpdateDisplayPalette( word snes_palette [256] )
{
              int color;
              
             //convertedPalette[0] = SDL_MapRGB( screen->format, 0,0,0 );//color 0 is BLACK COLOR ???
              
             if( PPU.Brightness == 0xF)
             {
		for( color = 0; color < 256; color++)
		{	
			word c = snes_palette [color];
	           		convertedPalette[color] = SDL_MapRGB( DISPLAY.screen->format,  
	           					/*   red  */  (Uint8) ( ( c  & 0x001F )  << 3 ) ,
	           					/* green*/  (Uint8) ( ( c  & 0x03E0 )  >> 2 ) ,
	           					/*  blue */   (Uint8) ( ( c  & 0x7C00 )  >> 7 ) );
		}
	}
	else
	{
		for( color = 0; color < 256; color++)
		{
			word c = snes_palette [color];
			Uint8 r , g, b;
			r =  (Uint8) (  (  (  ( c  & 0x001F)  << 3 ) * PPU.Brightness) / 0xf ) ;
			g = (Uint8) (  (  (  ( c  & 0x03E0)  >> 2 ) * PPU.Brightness) / 0xf ) ;
			b = (Uint8) (  (  (  ( c  & 0x7C00)  >> 7 ) * PPU.Brightness) / 0xf ) ;
	           		convertedPalette[color] = SDL_MapRGB(  DISPLAY.screen->format, r , g, b );
		}	
	}
}

/*
 call this function when one color in the snes palette is changed
*/
void UpdateDisplayPaletteColor( word color, int colorNumber )
{
             if( PPU.Brightness == 0xF)
             {
		convertedPalette[colorNumber] = SDL_MapRGB( DISPLAY.screen->format,  
	           				/*   red  */  (Uint8) ( ( color  & 0x001F )  << 3 ) ,
	           				/* green*/  (Uint8) ( ( color  & 0x03E0 )  >> 2 ) ,
	           				/*  blue */   (Uint8) ( ( color  & 0x7C00 )  >> 7 ) );
	}
	else
	{
		Uint8 r , g, b;
		r =  (Uint8) (  (  (  ( color  & 0x001F)  << 3 ) * PPU.Brightness) / 0xf ) ;
		g = (Uint8) (  (  (  ( color  & 0x03E0)  >> 2 ) * PPU.Brightness) / 0xf ) ;
		b = (Uint8) (  (  (  ( color  & 0x7C00)  >> 7 ) * PPU.Brightness) / 0xf ) ;
	           	convertedPalette[colorNumber] = SDL_MapRGB(  DISPLAY.screen->format, r , g, b );
	}
}
