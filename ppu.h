
#ifndef PPU_H
#define PPU_H

extern void resetppu(void);

extern void StartScreenRefresh (void);

extern void RenderLine (int visibleScanline);

extern void  EndScreenRefresh (void);

extern void UpdateScreen(void);

//remenber to call FLUSH_REDRAW() before the change of ppu registers values              
#define FLUSH_REDRAW()	 if (PPU.PreviousLine != PPU.CurrentLine)	 UpdateScreen ()


struct SOBJ
{
    short  HPos;
    word VPos;
    word Name;
    boolean  VFlip;
    boolean  HFlip;
    byte  Priority;
    byte  Palette;
    byte  Size;
};


extern struct ppu_settings{

  int screen_height;
  int screen_width;

  boolean ForcedBlanking;
  byte  Brightness;

 /*VRAM stuff*/
 byte VRAM[0x10000];//64k video ram

 struct {
	boolean High;
	byte Increment;
	word Address;
	word Mask1;
	word FullGraphicCount;
	word Shift;
    } VMA;
  boolean  FirstVRAMRead;

  /*IRQ, NMI, pos latch stuff*/
  boolean  HBeamFlip;
  boolean  VBeamFlip;

  word VBeamPosLatched;
  word HBeamPosLatched;

  word IRQHBeamPos;
  word IRQVBeamPos;
  int HTimerPosition;

  boolean VTimerEnabled;
  boolean HTimerEnabled;
  boolean NMIEnable;

 /*address to access wram*/
  word WRAM; 

 /*CGRAM stuff*/
 word CGDATA [256];//palette data
 boolean ColorsChanged;
 boolean CGFLIP;
 boolean CGFLIPRead;
 byte CGADD;

 /*OAM stuff*/
 byte  OAMData [512 + 32];
 boolean OBJChanged;
 byte  OAMPriorityRotation;
 word OAMAddr;
 word SavedOAMAddr;
 word OAMWriteRegister;
 byte  OAMFlip;

 /*OBJECTS stuff*/
 struct SOBJ OBJ [128];
 byte  OBJSizeSelect;
 word  OBJNameBase;
 word OBJNameSelect;
 byte  FirstSprite;
 byte  LastSprite;

 /*BG stuff*/
 byte  BGMode;
 byte  BG3Priority;
 struct {
	word SCBase;//to obtain the real address you have to shift left this address by 1
	word VOffset;
	word HOffset;
	byte BGSize;
	word NameBase;//to obtain the real address you have to shift left this address by 1
	word SCSize;
   } BG [4];
   byte  Mosaic;
   boolean  BGMosaic [4];
   
   /*Mode7 stuff*/
   boolean Mode7HFlip;
   boolean Mode7VFlip;
   byte Mode7Repeat;
   short  MatrixA;
   short  MatrixB;
   short  MatrixC;
   short  MatrixD;
   short  CentreX;
   short  CentreY;
   boolean  Need16x8Mulitply;
   byte Mode7PriorityMask;
   byte Mode7Mask;

 /*some dummy registers */
 byte reg4210;
 byte reg2100;
 byte reg2101;
 byte reg2105;
 byte reg2106;
 byte reg2107;
 byte reg2108;
 byte reg2109;
 byte reg210A;
 byte reg210B;
 byte reg210C;
 byte reg211A;
 byte reg212C;
 byte reg2133;
 
 /*some variables needed by rendering engine*/
 int PreviousLine;
 int CurrentLine;
 
  /*joypad internal variables*/
 dword Joypad1state;
 byte Joypad1ButtonReadPos;
 dword Joypad2state;
 byte Joypad2ButtonReadPos;
  
 /*frame skip stuff*/ 
  unsigned int frame_number;
  int frame_skip;
 
} PPU;

#define MAX_5C77_VERSION 0x01
#define MAX_5C78_VERSION 0x03
#define MAX_5A22_VERSION 0x02


extern struct SBG
{

    dword TileSize;
    dword BitShift;
    dword TileShift;
    dword TileAddress;
    dword NameSelect;
    dword SCBase;

    dword StartPalette;
    dword PaletteShift;
    dword PaletteMask;
    
    boolean  DirectColourMode;

    unsigned int zbuf_depth[4];
    unsigned int currentTileDepth;   
       
    byte *CacheBuffer;
    byte *CacheValidation;
    
}BG;

struct SLineData {
    struct {
	word VOffset;
	word HOffset;
    } BG [4];
};

struct SLineMatrixData
{
    short MatrixA;
    short MatrixB;
    short MatrixC;
    short MatrixD;
    short CentreX;
    short CentreY;
};

struct visibleObj {
    int	   OBJList [129];
    dword Sizes [129];
    int    VPositions [129];
};


#define TILE_2BIT 0
#define TILE_4BIT 1
#define TILE_8BIT 2



extern struct zbuf{
      unsigned int depth[SNES_SCREEN_HEIGHT_EXTENDED][SNES_SCREEN_WIDTH];
      unsigned int current_base_depth;
} z_buffer;

#define BG2_DEPTH_EXTRA_PRIORITY	14
#define OBJ_DEPTH_PRIORITY3		13
#define BG0_DEPTH_WITH_PRIORITY	12
#define BG1_DEPTH_WITH_PRIORITY	11
#define OBJ_DEPTH_PRIORITY2		10	
#define BG1_DEPTH_EXTRA_PRIORITY	9
#define BG0_DEPTH_NO_PRIORITY	8
#define BG1_DEPTH_NO_PRIORITY	7
#define OBJ_DEPTH_PRIORITY1		6
#define BG2_DEPTH_WITH_PRIORITY	5 
#define BG3_DEPTH_WITH_PRIORITY	4
#define OBJ_DEPTH_PRIORITY0		3
#define BG2_DEPTH_NO_PRIORITY	2
#define BG3_DEPTH_NO_PRIORITY	1

#define MAX_DEPTH	BG2_DEPTH_EXTRA_PRIORITY
#define BASE_DEPTH_RESET	0xFFFFFF

#endif//end of library
