
#ifndef DISPLAY_H
#define DISPLAY_H

extern struct display_setting
{
 	int bpl;//byte per line
	
	int bpp;//byte per pixel                      
	
	int zoom;//the actual zoom of the snes screen
	
} SCREEN;

/*
  Initialize the Video and the input environment used by emulator
  return value:
	0 if ok
            -1 if an error occur
*/
extern int initVideo(void);

/*        
 this function map the snes palette with a palette suitable for screen rendering
 call this function when snes palette change
*/
extern void UpdateDisplayPalette( word snes_palette [256] );

/*
 call this function when one color in the snes palette is changed
*/
extern void UpdateDisplayPaletteColor( word color, int colorNumber );

/*
  call this function when the snes need to know the joypad status
  Also handle emulator exit
  - joypad is the joypad number (possible value 0-3)
  Notice that you have to call this function for joypad 0 before others pads because is the call to joypad 0
  that refresh the input.
*/
extern word getjoypad(int joypad);


/*Call this before the rendering of the current frame. i.e. before using putpixel() function*/
extern void clearScreen(void);

/*
 when the snes screen is ready to be displayed call this function
*/
extern void renderSnesScreen(void);


#define renderTile(data,paletteOffset ,X,Y,StartPixel,Width,StartLine,LineCount)  	(*prenderTile) \
					(data,paletteOffset ,X,Y,StartPixel,Width,StartLine,LineCount)
extern void (*prenderTile)(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount);

#define renderTileVflip(data,paletteOffset ,X,Y,StartPixel,Width,StartLine,LineCount)	(*prenderTileVflip)\
						(data,paletteOffset ,X,Y,StartPixel,Width,StartLine,LineCount)
extern void (*prenderTileVflip)(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount);

#define renderTileHflip(data,paletteOffset ,X,Y,StartPixel,Width,StartLine,LineCount)	(*prenderTileHflip)\
						(data,paletteOffset ,X,Y,StartPixel,Width,StartLine,LineCount)
extern void (*prenderTileHflip)(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount);

#define renderTileHVflip(data,paletteOffset ,X,Y,StartPixel,Width,StartLine,LineCount)	(*prenderTileHVflip)\
						(data,paletteOffset ,X,Y,StartPixel,Width,StartLine,LineCount)
extern void (*prenderTileHVflip)(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount);
   
   
/*
 This functions draw the pixel passed as argument in the screen.
 x e y is the snes screen coordinate and pixel is the value of the pixel(the palette index)
 Another way to set the pixel is to use the offset value. The offset is a byte
 offset of a frame buffer and it is calculate referring to the SCREEN structure.
 This structure has a bpp field that is bytes per pixel and a bpl that is bytes per line.
 If you have set correctly this values then the offset passed to this function is the byte address
 in the frame buffer where the pixel needs to be drawn.
 Defining putpixel as a macro allow you to call an apropriate function that use the x and y coordinate or
 a function that use offset value as you prefer.
*/

#define putpixel(offset,x,y,pixel)	(*pputpixelOffset)(offset,pixel)
extern void (*pputpixelOffset)(dword offset, byte pixel);

//call this at the beginnig of the frame. 
extern void syncSpeed(void);   

#define DISPLAY_WIDTH  	512
#define DISPLAY_HEIGHT 	478


#define WINDOW_TITLE "SENSE...Linux is great :)"

#define ICON_NAME    "sense.bmp"


#endif  //end of library
