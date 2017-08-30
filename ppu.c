
#include "general.h"
#include "ppu.h"
#include "display.h"
#include "tileConverter.h"

struct ppu_settings PPU;

struct SBG BG;//hold the information of the currnet background                                                                        

struct SLineData LineData[240];//for each line this array store the vertical and horizontal offset for each background

struct SLineMatrixData LineMatrixData [240];//for each line this array store the mode 7 parameter

struct visibleObj VOBJ;//mantain the informations for the visible object

struct zbuf z_buffer;

void resetZbuffer(void)
{
  int i,j;
    
   for(j=0; j<SNES_SCREEN_HEIGHT_EXTENDED; j++)
      for(i=0; i<SNES_SCREEN_WIDTH; i++)  
                z_buffer.depth[j][i] = 0;
            
   z_buffer.current_base_depth = 0;
}

void resetppu(void){

 	PPU.screen_height = SNES_SCREEN_HEIGHT;
	PPU.screen_width = SNES_SCREEN_WIDTH;
	
	PPU.ForcedBlanking = TRUE;
	PPU.Brightness = 0;
	
	PPU.VMA.High = 0;
	PPU.VMA.Increment = 1;
	PPU.VMA.Address = 0;
	PPU.VMA.FullGraphicCount = 0;
	PPU.VMA.Shift = 0;

	PPU.FirstVRAMRead = FALSE;

 	PPU.HBeamFlip = 0;
	PPU.VBeamFlip = 0;

 	PPU.VBeamPosLatched = 0;
 	PPU.HBeamPosLatched = 0;

 	PPU.IRQHBeamPos = 0;
 	PPU.IRQVBeamPos = 0;
 	PPU.HTimerPosition = 0;

 	PPU.VTimerEnabled = FALSE;
 	PPU.HTimerEnabled = FALSE;
 	PPU.NMIEnable = FALSE;

	PPU.WRAM = 0;

	PPU.ColorsChanged = TRUE;
 	PPU.CGFLIP = 0;
 	PPU.CGFLIPRead = 0;
 	PPU.CGADD = 0;

	PPU.OBJChanged = TRUE;
              PPU.OAMPriorityRotation = 0;
	PPU.OAMAddr = 0;
	PPU.SavedOAMAddr = 0;
	PPU.OAMWriteRegister = 0;
	PPU.OAMFlip = 0;

	PPU.OBJSizeSelect = 0;
              PPU.OBJNameBase = 0;
 	PPU.OBJNameSelect = 0;
	PPU.FirstSprite = 0;
    	PPU.LastSprite = 127;
	/*reset OBJ*/
	{int Sprite;
	for (Sprite = 0; Sprite < 128; Sprite++)
	{
		PPU.OBJ[Sprite].HPos = 0;
		PPU.OBJ[Sprite].VPos = 0;
		PPU.OBJ[Sprite].VFlip = 0;
		PPU.OBJ[Sprite].HFlip = 0;
		PPU.OBJ[Sprite].Priority = 0;
		PPU.OBJ[Sprite].Palette = 0;
		PPU.OBJ[Sprite].Name = 0;
		PPU.OBJ[Sprite].Size = 0;
	}
	}

	PPU.BGMode = 0;
	PPU.BG3Priority = 0;
	/*reset BGs*/
	{byte B;
	for ( B = 0; B != 4; B++)
	{
		PPU.BG[B].SCBase = 0;
		PPU.BG[B].VOffset = 0;
		PPU.BG[B].HOffset = 0;
		PPU.BG[B].BGSize = 0;
		PPU.BG[B].NameBase = 0;
		PPU.BG[B].SCSize = 0;
	}
	}
	PPU.Mosaic = 0;
	PPU.BGMosaic [0] = PPU.BGMosaic [1] = FALSE;
	PPU.BGMosaic [2] = PPU.BGMosaic [3] = FALSE;

	PPU.Mode7HFlip = FALSE;
    	PPU.Mode7VFlip = FALSE;
    	PPU.Mode7Repeat = 0;
	PPU.MatrixA = 0;
   	PPU.MatrixB = 0;
   	PPU.MatrixC = 0;
   	PPU.MatrixD = 0;
   	PPU.CentreX = 0;
   	PPU.CentreY = 0;
   	PPU.Need16x8Mulitply = FALSE;
	PPU.Mode7PriorityMask = 0x00;
	PPU.Mode7Mask = 0xFF;

 	
 	PPU.reg4210 = 0;
 	PPU.reg2100 = 0x80;
 	PPU.reg2101 = 0;
 	PPU.reg2105 = 0;
 	PPU.reg2106 = 0;
	PPU.reg2107 = 0;
	PPU.reg2108 = 0;
 	PPU.reg2109 = 0;
	PPU.reg210A = 0;
	PPU.reg210B = 0;
	PPU.reg210C = 0;
	PPU.reg211A = 0;
	PPU.reg212C = 0;
	PPU.reg2133 = 0;

		
	PPU.PreviousLine = 0;
     	PPU.CurrentLine = 0;
     	
     	PPU.Joypad1state = 0;
 	PPU.Joypad1ButtonReadPos =0;
 	PPU.Joypad2state =0;
 	PPU.Joypad2ButtonReadPos =0;
 	
 	PPU.frame_number = 0;
	PPU.frame_skip = 1;
 	
 	resetTileConverter();
 	
 	resetZbuffer();
}


/* the color depth. number of bits per color( equal to the number of planes of the tile)*/
byte BitShifts[8][4] =
{
    {2, 2, 2, 2},	// 0
    {4, 4, 2, 0},	// 1
    {4, 4, 0, 0},	// 2
    {8, 4, 0, 0},	// 3
    {8, 2, 0, 0},	// 4
    {4, 2, 0, 0},	// 5
    {4, 0, 0, 0},	// 6
    {8, 0, 0, 0}	// 7
};

/*
  once you have the tile number and the tile base address you have to shift the
  tile number by the value of this table(and then add the base address) to get the
  starting byte of the tile
*/
byte TileShifts[8][4] =
{
    {4, 4, 4, 4},	// 0
    {5, 5, 4, 0},	// 1
    {5, 5, 0, 0},	// 2
    {6, 5, 0, 0},	// 3
    {6, 4, 0, 0},	// 4
    {5, 4, 0, 0},	// 5
    {5, 0, 0, 0},	// 6
    {6, 0, 0, 0}	// 7
};

/*
  once you get the tile colors you have to shift the palette value by the value of this table
  to get the right color in the palette
*/
byte PaletteShifts[8][4] =
{
    {2, 2, 2, 2},	// 0
    {4, 4, 2, 0},	// 1
    {4, 4, 0, 0},	// 2
    {0, 4, 0, 0},	// 3
    {0, 2, 0, 0},	// 4
    {4, 2, 0, 0},	// 5
    {4, 0, 0, 0},	// 6
    {0, 0, 0, 0}	// 7
};

/*to retrieve the palette bits in the tile informations*/
byte PaletteMasks[8][4] =
{
    {7, 7, 7, 7},	// 0
    {7, 7, 7, 0},	// 1
    {7, 7, 0, 0},	// 2
    {0, 7, 0, 0},	// 3
    {0, 7, 0, 0},	// 4
    {7, 7, 0, 0},	// 5
    {7, 0, 0, 0},	// 6
    {0, 0, 0, 0}	// 7
};

/*8x8 or 16x16 tile size*/
byte BGSizes [2] = {
    8, 16
};


byte Depths[8][4] =
{
    {TILE_2BIT, TILE_2BIT, TILE_2BIT, TILE_2BIT}, // 0
    {TILE_4BIT, TILE_4BIT, TILE_2BIT, 0},         // 1
    {TILE_4BIT, TILE_4BIT, 0, 0},                 // 2
    {TILE_8BIT, TILE_4BIT, 0, 0},                 // 3
    {TILE_8BIT, TILE_2BIT, 0, 0},                 // 4
    {TILE_4BIT, TILE_2BIT, 0, 0},                 // 5
    {TILE_8BIT, 0, 0, 0},                         // 6
    {0, 0, 0, 0}                                  // 7
};

   /*
void BuildDirectColourMaps ()
{
    //palette mean the 3 bits palette index of each tile
    for (uint32 palette = 0; palette < 8; palette++)
    {
    	//c is the color index of each tile
	for (uint32 c = 0; c < 256; c++)
	{
// XXX: Brightness
	    DirectColourMaps [palette][c] = BUILD_PIXEL (((c & 7) << 2) | ((palette & 1) << 1),
						   ((c & 0x38) >> 1) | (palette & 2),
						   ((c & 0xc0) >> 3) | (palette & 4));
	}
    }
    PPU.DirectColourMapsNeedRebuild = FALSE;
}
   */

void StartScreenRefresh (void)
{
    syncSpeed();   

    if( PPU.frame_number % PPU.frame_skip ) return;
    
    PPU.PreviousLine = PPU.CurrentLine = 0;
    clearScreen();
}
      
void RenderLine (int visibleScanline)
{
	if( PPU.frame_number % PPU.frame_skip ) return;
	
	LineData[ visibleScanline].BG[0].VOffset = PPU.BG[0].VOffset+1;
	LineData[ visibleScanline].BG[0].HOffset = PPU.BG[0].HOffset;
	LineData[ visibleScanline].BG[1].VOffset = PPU.BG[1].VOffset+1;
	LineData[ visibleScanline].BG[1].HOffset = PPU.BG[1].HOffset;

	if (PPU.BGMode == 7)
	{
	    struct SLineMatrixData *p = &LineMatrixData [visibleScanline];
	    p->MatrixA = PPU.MatrixA;
	    p->MatrixB = PPU.MatrixB;
	    p->MatrixC = PPU.MatrixC;
	    p->MatrixD = PPU.MatrixD;
	    p->CentreX = PPU.CentreX;
	    p->CentreY = PPU.CentreY;
	}
	else
	{
		LineData[ visibleScanline].BG[2].VOffset = PPU.BG[2].VOffset+1;
		LineData[ visibleScanline].BG[2].HOffset = PPU.BG[2].HOffset;
		LineData[ visibleScanline].BG[3].VOffset = PPU.BG[3].VOffset+1;
		LineData[ visibleScanline].BG[3].HOffset = PPU.BG[3].HOffset;

	}
	PPU.CurrentLine = visibleScanline + 1;
}

void  EndScreenRefresh (void)
{
     if( PPU.frame_number++ % PPU.frame_skip ) return;
     
     FLUSH_REDRAW();
     renderSnesScreen();
     
     z_buffer.current_base_depth += MAX_DEPTH;
    if( z_buffer.current_base_depth > BASE_DEPTH_RESET )
		          	resetZbuffer();
		        
}

void DrawBackground (byte BGMode, byte bg);

void DrawBackgroundMode7 (void);
 
void SetupOBJ (void);

void DrawOBJS (void);

void UpdateScreen(void)
{
    if( ! PPU.ForcedBlanking){
    
    	if ( PPU.ColorsChanged )
	{
		UpdateDisplayPalette( PPU.CGDATA  );
		PPU.ColorsChanged = FALSE;
	}
	
    	if ( PPU.OBJChanged )
    	{
		SetupOBJ ();
		PPU.OBJChanged = FALSE;
              }
              
    	//if (PPU.RecomputeClipWindows)
    	//{
	//	ComputeClipWindows ();
	//	PPU.RecomputeClipWindows = FALSE;
    	//}
	
    
    	switch (PPU.BGMode)
    	{
    	case 0:
    	 	if( PPU.reg212C & 16)
    	 	{
   			BG.zbuf_depth[0] = OBJ_DEPTH_PRIORITY0+z_buffer.current_base_depth;
   			BG.zbuf_depth[1] = OBJ_DEPTH_PRIORITY1+z_buffer.current_base_depth;
			BG.zbuf_depth[2] = OBJ_DEPTH_PRIORITY2+z_buffer.current_base_depth;
   			BG.zbuf_depth[3] = OBJ_DEPTH_PRIORITY3+z_buffer.current_base_depth;
	    	 	DrawOBJS();                            
	    	 }
    	 	
    	 	if( PPU.reg212C & 1)
    	 	{
   			BG.zbuf_depth[0] = BG0_DEPTH_NO_PRIORITY+z_buffer.current_base_depth;
			BG.zbuf_depth[1] = BG0_DEPTH_WITH_PRIORITY+z_buffer.current_base_depth;
    	        		DrawBackground (PPU.BGMode, 0);
    	        	}
    	        	
      	 	if( PPU.reg212C & 2)
      	 	{
   			BG.zbuf_depth[0] = BG1_DEPTH_NO_PRIORITY+z_buffer.current_base_depth;
			BG.zbuf_depth[1] = BG1_DEPTH_WITH_PRIORITY+z_buffer.current_base_depth;
    	        		DrawBackground (PPU.BGMode, 1);
    	        	}

    	        	if( PPU.reg212C & 4)
    	        	{
   			BG.zbuf_depth[0] = BG2_DEPTH_NO_PRIORITY+z_buffer.current_base_depth;
			BG.zbuf_depth[1] = BG2_DEPTH_WITH_PRIORITY+z_buffer.current_base_depth;
    	      		DrawBackground (PPU.BGMode, 2);          
    	      	}
    	      		 
   		if( PPU.reg212C & 8)
   		{
   			BG.zbuf_depth[0] = BG3_DEPTH_NO_PRIORITY+z_buffer.current_base_depth;
			BG.zbuf_depth[1] = BG3_DEPTH_WITH_PRIORITY+z_buffer.current_base_depth;
	       		DrawBackground (PPU.BGMode, 3);    	
	       	}
	       	
    		break;
  		
   	case 1: 
 	 	if( PPU.reg212C & 16)
    	 	{
   			BG.zbuf_depth[0] = OBJ_DEPTH_PRIORITY0+z_buffer.current_base_depth;
   			BG.zbuf_depth[1] = OBJ_DEPTH_PRIORITY1+z_buffer.current_base_depth;
			BG.zbuf_depth[2] = OBJ_DEPTH_PRIORITY2+z_buffer.current_base_depth;
   			BG.zbuf_depth[3] = OBJ_DEPTH_PRIORITY3+z_buffer.current_base_depth;
	    	 	DrawOBJS();                            
	    	 }
    	 	
    	 	if( PPU.reg212C & 1)
    	 	{
   			BG.zbuf_depth[0] = BG0_DEPTH_NO_PRIORITY+z_buffer.current_base_depth;
			BG.zbuf_depth[1] = BG0_DEPTH_WITH_PRIORITY+z_buffer.current_base_depth;
    	        		DrawBackground (PPU.BGMode, 0);
    	        	}
    	        	
      	 	if( PPU.reg212C & 2)
      	 	{             
			if( PPU.BG3Priority )
    	        		{
   			BG.zbuf_depth[0] = BG1_DEPTH_NO_PRIORITY+z_buffer.current_base_depth;
			BG.zbuf_depth[1] = BG1_DEPTH_EXTRA_PRIORITY+z_buffer.current_base_depth;
    	        		}
    	        		else
    	        		{
			BG.zbuf_depth[0] = BG1_DEPTH_NO_PRIORITY+z_buffer.current_base_depth;
			BG.zbuf_depth[1] = BG1_DEPTH_WITH_PRIORITY+z_buffer.current_base_depth;
			}
    	        		DrawBackground (PPU.BGMode, 1);
    	        	}

    	        	if( PPU.reg212C & 4)
    	        	{
    	        		if( PPU.BG3Priority )
    	        		{
   			BG.zbuf_depth[0] = BG2_DEPTH_NO_PRIORITY+z_buffer.current_base_depth;
			BG.zbuf_depth[1] = BG2_DEPTH_EXTRA_PRIORITY+z_buffer.current_base_depth;
    	        		}
    	        		else
    	        		{
   			BG.zbuf_depth[0] = BG2_DEPTH_NO_PRIORITY+z_buffer.current_base_depth;
			BG.zbuf_depth[1] = BG2_DEPTH_WITH_PRIORITY+z_buffer.current_base_depth;
    	        		}
    	      		DrawBackground (PPU.BGMode, 2);          
    	      	}
    		break;
  	
    	case 3:  case 2: case 4:  case 5: case 6:      
    		if( PPU.reg212C & 16)
    	 	{
   			BG.zbuf_depth[0] = OBJ_DEPTH_PRIORITY0+z_buffer.current_base_depth;
   			BG.zbuf_depth[1] = OBJ_DEPTH_PRIORITY1+z_buffer.current_base_depth;
			BG.zbuf_depth[2] = OBJ_DEPTH_PRIORITY2+z_buffer.current_base_depth;
   			BG.zbuf_depth[3] = OBJ_DEPTH_PRIORITY3+z_buffer.current_base_depth;
	    	 	DrawOBJS();                            
	    	 }
    	 	
    	 	if( PPU.reg212C & 1)
    	 	{
   			BG.zbuf_depth[0] = BG0_DEPTH_NO_PRIORITY+z_buffer.current_base_depth;
			BG.zbuf_depth[1] = BG0_DEPTH_WITH_PRIORITY+z_buffer.current_base_depth;
    	        		DrawBackground (PPU.BGMode, 0);
    	        	}
    	        	
      	 	if( PPU.reg212C & 2)
      	 	{
   			BG.zbuf_depth[0] = BG1_DEPTH_NO_PRIORITY+z_buffer.current_base_depth;
			BG.zbuf_depth[1] = BG1_DEPTH_WITH_PRIORITY+z_buffer.current_base_depth;
    	        		DrawBackground (PPU.BGMode, 1);
    	        	}

    		break;
/*    	case 2:    	
    	case 4: // Used by Puzzle Bobble
        		break;

    	case 5:
    	case 6: // XXX: is also offset per tile.
		break;*/
    	case 7:
    	    	if( PPU.reg212C & 16)
    	 	{
   			BG.zbuf_depth[0] = OBJ_DEPTH_PRIORITY0+z_buffer.current_base_depth;
   			BG.zbuf_depth[1] = OBJ_DEPTH_PRIORITY1+z_buffer.current_base_depth;
			BG.zbuf_depth[2] = OBJ_DEPTH_PRIORITY2+z_buffer.current_base_depth;
   			BG.zbuf_depth[3] = OBJ_DEPTH_PRIORITY3+z_buffer.current_base_depth;
	    	 	DrawOBJS();                            
	    	 }
    	 	
	    	if( PPU.reg212C & 1)
    	 	{
			BG.zbuf_depth[0] = BG0_DEPTH_NO_PRIORITY+z_buffer.current_base_depth;
			BG.zbuf_depth[1] = BG0_DEPTH_WITH_PRIORITY+z_buffer.current_base_depth;
    			DrawBackgroundMode7 ();
    		}
                            break;
    	}
    }

   PPU.PreviousLine = PPU.CurrentLine;

}



void DrawBackground (byte BGMode, byte bg)
{
    dword Tile;
    word *SC0;
    word *SC1;
    word *SC2;
    word *SC3;
    dword Width;

    int X,Y;
    int Lines;
    int OffsetMask;
    int OffsetShift;

    BG.TileSize = BGSizes [PPU.BG[bg].BGSize];
    BG.BitShift = BitShifts[BGMode][bg];
    BG.TileShift = TileShifts[BGMode][bg];
    BG.TileAddress = PPU.BG[bg].NameBase << 1;
    BG.NameSelect = 0;
    BG.PaletteShift = PaletteShifts[BGMode][bg];
    BG.PaletteMask = PaletteMasks[BGMode][bg];
    BG.CacheBuffer = &TileCache.data [Depths [BGMode][bg]][0];
    BG.CacheValidation = &TileCache.valid [Depths [BGMode][bg]][0];
    //BG.DirectColourMode = (BGMode == 3 || BGMode == 4) && bg == 0 &&
    //		(GFX.r2130 & 1);
    BG.DirectColourMode = FALSE;

   if (BGMode == 0)
		BG.StartPalette = bg << 5;
   else BG.StartPalette = 0;

    SC0 = (word *) &PPU.VRAM[PPU.BG[bg].SCBase << 1];

    if (PPU.BG[bg].SCSize & 1)
		SC1 = SC0 + 1024;
    else
		SC1 = SC0;

    if (PPU.BG[bg].SCSize & 2)
		SC2 = SC1 + 1024;
    else
		SC2 = SC0;

    if (PPU.BG[bg].SCSize & 1)
		SC3 = SC2 + 1024;
    else
		SC3 = SC2;

    if (BG.TileSize == 16)
    {
		OffsetMask = 0x3ff;
		OffsetShift = 4;
    }
    else
    {
		OffsetMask = 0x1ff;
		OffsetShift = 3;
    }

    for ( Y = PPU.PreviousLine; Y < PPU.CurrentLine; Y += Lines)
    {
		dword VOffset = LineData[Y].BG[bg].VOffset;
		dword HOffset = LineData[Y].BG[bg].HOffset;
		int VirtAlign = (Y + VOffset) & 7;

		for (Lines = 1; Lines < 8 - VirtAlign; Lines++)
			if ((VOffset != LineData [Y + Lines].BG[bg].VOffset) ||
				(HOffset != LineData [Y + Lines].BG[bg].HOffset))
				break;

			if (Y + Lines > PPU.CurrentLine)
				Lines = PPU.CurrentLine - Y;
				
			VirtAlign <<= 3;

                            {
			dword ScreenLine = (VOffset + Y) >> OffsetShift;
			dword t1;
			dword t2;
			word *b1;
			word *b2;

			if (((VOffset + Y) & 15) > 7)
			{
				t1 = 16;
				t2 = 0;
			}
			else
			{
				t1 = 0;
				t2 = 16;
			}

			if (ScreenLine & 0x20)
				b1 = SC2, b2 = SC3;
			else
				b1 = SC0, b2 = SC1;

			b1 += (ScreenLine & 0x1f) << 5;
			b2 += (ScreenLine & 0x1f) << 5;

			{
				dword Left = 0;
				dword Right = 256;

				//if (!GFX.pCurrentClip->Count [bg])
				//{
				//	Left = 0;
				//	Right = 256;
				//}
				//else
				//{
				//	Left = GFX.pCurrentClip->Left [clip][bg];
				//	Right = GFX.pCurrentClip->Right [clip][bg];

				//	if (Right <= Left)
				//		continue;
				//}
                                                        //dword p = Left * SCREEN.bpp + Y * SCREEN.bpl;
				dword HPos = (HOffset + Left) & OffsetMask;

				dword Quot = HPos >> 3;
				dword Count = 0;
				int Middle;
				word *t;
				
				X = Left;
                                                        
				if (BG.TileSize == 8)
				{
					if (Quot > 31)
						t = b2 + (Quot & 0x1f);
					else
						t = b1 + Quot;
				}
				else
				{
					if (Quot > 63)
						t = b2 + ((Quot >> 1) & 0x1f);
					else
						t = b1 + (Quot >> 1);
				}
				
				Width = Right - Left;
				 // Left hand edge clipped tile
				if (HPos & 7)
				{
					dword Offset = (HPos & 7);
					Count = 8 - Offset;
					if (Count > Width)
						Count = Width;

					Tile = READ_2BYTES(t);
                                                                      BG.currentTileDepth = BG.zbuf_depth[(Tile & 0x2000) >> 13];

					if (BG.TileSize == 8)
					{
						DrawTile (Tile,  X , Y, Offset, Count, VirtAlign, Lines );
					}
					else
					{
						if (!(Tile & (V_FLIP | H_FLIP)))
						{
							// Normal, unflipped
							DrawTile (Tile + t1 + (Quot & 1),  X , Y, Offset, Count, VirtAlign, Lines );
						}
						else
							if (Tile & H_FLIP)
							{
								if (Tile & V_FLIP)
								{
									// H & V flip
									DrawTile (Tile + t2 + 1 - (Quot & 1),  X , Y, Offset, Count, VirtAlign, Lines );
								}
								else
								{
									// H flip only
									DrawTile (Tile + t1 + 1 - (Quot & 1),  X , Y, Offset, Count, VirtAlign, Lines );
								}
							}
							else
							{
								// V flip only
								DrawTile (Tile + t2 + (Quot & 1),  X , Y, Offset, Count, VirtAlign, Lines );
							}
					}

					if (BG.TileSize == 8)
					{
						t++;
						if (Quot == 31)
							t = b2;
						else if (Quot == 63)
							t = b1;
					}
					else
					{
						t += Quot & 1;
						if (Quot == 63)
							t = b2;
						else if (Quot == 127)
							t = b1;
					}
					Quot++;
					X += (8 - Offset);
					//p += (8 - Offset) * SCREEN.bpp;
				}

				// Middle, unclipped tiles
				Count = Width - Count;
				Middle = Count >> 3;
				Count &= 7;
				{ int C;
				for ( C = Middle; C > 0; Quot++, X+=8, C--/*, p +=(SCREEN.bpp<<3) */)
				{
					Tile = READ_2BYTES(t);
                                                                      BG.currentTileDepth = BG.zbuf_depth[(Tile & 0x2000) >> 13];
                                                                        
					if (BG.TileSize != 8)
					{
						if (Tile & H_FLIP)
						{
							// Horizontal flip, but what about vertical flip ?
							if (Tile & V_FLIP)
							{
								// Both horzontal & vertical flip
								DrawTile (Tile + t2 + 1 - (Quot & 1), X,Y,0,8, VirtAlign, Lines );
							}
							else
							{
								// Horizontal flip only
								DrawTile (Tile + t1 + 1 - (Quot & 1), X,Y,0,8, VirtAlign, Lines );
							}
						}
						else
						{
							// No horizontal flip, but is there a vertical flip ?
							if (Tile & V_FLIP)
							{
								// Vertical flip only
								DrawTile (Tile + t2 + (Quot & 1), X,Y,0,8, VirtAlign, Lines );
							}
							else
							{
								// Normal unflipped
								DrawTile (Tile + t1 + (Quot & 1),  X, Y,0,8, VirtAlign, Lines );
							}
						}
					}
					else
					{                             
						DrawTile (Tile, X , Y,0,8, VirtAlign, Lines );
					}
					
					if (BG.TileSize == 8)
					{
						t++;
						if (Quot == 31)
							t = b2;
						else
							if (Quot == 63)
								t = b1;
					}
					else
					{
						t += Quot & 1;
						if (Quot == 63)
							t = b2;
						else
							if (Quot == 127)
								t = b1;
					}
				}
				}
				// Right-hand edge clipped tiles
				if (Count)
				{
					Tile = READ_2BYTES(t);
					BG.currentTileDepth = BG.zbuf_depth[(Tile & 0x2000) >> 13];

					if (BG.TileSize == 8)
					{
						DrawTile (Tile,  X , Y, 0, Count, VirtAlign, Lines );
					}
					else
					{
						if (!(Tile & (V_FLIP | H_FLIP)))
						{
							// Normal, unflipped
							DrawTile (Tile + t1 + (Quot & 1),  X , Y, 0, Count, VirtAlign, Lines );
						}
						else
							if (Tile & H_FLIP)
							{
								if (Tile & V_FLIP)
								{
									// H & V flip
									DrawTile (Tile + t2 + 1 - (Quot & 1),  X , Y, 0, Count, VirtAlign, Lines );
								}
								else
								{
									// H flip only
									DrawTile (Tile + t1 + 1 - (Quot & 1),  X , Y, 0, Count, VirtAlign, Lines );
								}
							}
							else
							{
								// V flip only
								DrawTile (Tile + t2 + (Quot & 1),  X , Y, 0, Count, VirtAlign, Lines );
							}
					}
				}
				
			}
		}
	}
}

#define CLIP_10_BIT_SIGNED(a) \
	((a) & ((1 << 10) - 1)) + (((((a) & (1 << 13)) ^ (1 << 13)) - (1 << 13)) >> 3)

#define M7 19

void DrawBackgroundMode7 (void)
{
    byte *VRAM1 = PPU.VRAM + 1; 
    
/*    if (GFX.r2130 & 1) 
    { 
	if (IPPU.DirectColourMapsNeedRebuild) 
	    S9xBuildDirectColourMaps (); 
	GFX.ScreenColors = DirectColourMaps [0]; 
    } 
    else 
	GFX.ScreenColors = IPPU.ScreenColors; */

    int aa, cc; 
    int dir; 
    int startx, endx; 
    dword Left = 0; 
    dword Right = 256; 
    
    dword lineOffset = PPU.PreviousLine * SCREEN.bpl;//it's the address of the beginning current line in the frame buffer
    dword p; //it's the address of the current pixel in the frame buffer
    struct SLineMatrixData *l = &LineMatrixData [PPU.PreviousLine]; 
    dword Line;
    for (Line = PPU.PreviousLine; Line < PPU.CurrentLine; Line++, l++, lineOffset +=SCREEN.bpl ) 
    { 
	int yy,xx; 
	int AA,BB,CC,DD;

	sdword HOffset = ((sdword) LineData [Line].BG[0].HOffset << M7) >> M7; 
	sdword VOffset = ((sdword) LineData [Line].BG[0].VOffset << M7) >> M7; 

	sdword CentreX = ((sdword) l->CentreX << M7) >> M7; 
	sdword CentreY = ((sdword) l->CentreY << M7) >> M7; 

	if (PPU.Mode7VFlip) 
	    yy = 255 - (int) Line; 
	else 
	    yy = Line; 

    	yy += CLIP_10_BIT_SIGNED(VOffset - CentreY); 

  	BB = l->MatrixB * yy + (CentreX << 8); 
	DD = l->MatrixD * yy + (CentreY << 8); 
              	
	{ 
	    p = lineOffset + (Left * SCREEN.bpp );
	    
	    if (PPU.Mode7HFlip) 
	    { 
		startx = Right - 1; 
		endx = Left - 1; 
		dir = -1; 
		aa = -l->MatrixA; 
		cc = -l->MatrixC; 
	    } 
	    else 
	    { 
		startx = Left; 
		endx = Right; 
		dir = 1; 
		aa = l->MatrixA; 
		cc = l->MatrixC; 
	    } 

	   xx = startx + CLIP_10_BIT_SIGNED(HOffset - CentreX); 
	   AA = l->MatrixA * xx; 
	   CC = l->MatrixC * xx; 

    	    if (!PPU.Mode7Repeat) 
	    { 
	     	int x;
		for (x = startx; x != endx; x += dir, AA += aa, CC += cc, p+=SCREEN.bpp ) 
		{ 		
		    int X = ((AA + BB) >> 8) & 0x3ff; 
		    int Y = ((CC + DD) >> 8) & 0x3ff; 
		    byte *TileData = VRAM1 + ( PPU.VRAM[((Y & ~7) << 5) + ((X >> 2) & ~1)] << 7); 
		    byte b = *(TileData + ((Y & 7) << 4) + ((X & 7) << 1)); 
		    BG.currentTileDepth = BG.zbuf_depth[ (b & PPU.Mode7PriorityMask) >> 7]; 
		    
		    if( (BG.currentTileDepth > z_buffer.depth[Line][x]) &&
		       		( b & PPU.Mode7Mask) )
		     {
		   	putpixel(p,x,Line,b& PPU.Mode7Mask);
   			z_buffer.depth[Line][x] = BG.currentTileDepth;
		     }
		} 
	    } 
	    
	    else 
	    {
	              int x;
	              byte b;
		for (x = startx; x != endx; x += dir, AA += aa, CC += cc, p+=SCREEN.bpp) 
		{ 
		    int X = ((AA + BB) >> 8); 
		    int Y = ((CC + DD) >> 8); 

		    if (((X | Y) & ~0x3ff) == 0) 
		    { 
			byte *TileData = VRAM1 + (PPU.VRAM[((Y & ~7) << 5) + ((X >> 2) & ~1)] << 7); 
			b = *(TileData + ((Y & 7) << 4) + ((X & 7) << 1)); 
			
			BG.currentTileDepth = BG.zbuf_depth[ (b & PPU.Mode7PriorityMask) >> 7]; 
		    
		    	if( (BG.currentTileDepth > z_buffer.depth[Line][x]) &&
		       		( b & PPU.Mode7Mask) )
		     	{
		   		putpixel(p,x,Line,b& PPU.Mode7Mask);
   				z_buffer.depth[Line][x] = BG.currentTileDepth;
		     	}
		    } 
		    else 
		    { 
			if (PPU.Mode7Repeat == 3) 
			{ 
			    X = (x + HOffset) & 7; 
			    Y = (yy + CentreY) & 7; 
			    b = *(VRAM1 + ((Y & 7) << 4) + ((X & 7) << 1)); 
			    
			    BG.currentTileDepth = BG.zbuf_depth[ (b & PPU.Mode7PriorityMask) >> 7]; 
		    
			    if( (BG.currentTileDepth > z_buffer.depth[Line][x]) &&
		       		( b & PPU.Mode7Mask) )
		     	   {
		   		putpixel(p,x,Line,b& PPU.Mode7Mask);
   				z_buffer.depth[Line][x] = BG.currentTileDepth;
		                 }
		                 
			} 
		    } 
		}
	    }  
	} 
   }
             
}

void SetupOBJ (void)
{
    int SmallSize;
    int LargeSize;
    int C = 0;
    int FirstSprite = PPU.FirstSprite & 0x7f;
    int S = FirstSprite;


    switch (PPU.OBJSizeSelect)
    {
    case 0:
	SmallSize = 8;
	LargeSize = 16;
	break;
    case 1:
	SmallSize = 8;
	LargeSize = 32;
	break;
    case 2:
	SmallSize = 8;
	LargeSize = 64;
	break;
    case 3:
	SmallSize = 16;
	LargeSize = 32;
	break;
    case 4:
	SmallSize = 16;
	LargeSize = 64;
	break;
    case 5:
    default:
	SmallSize = 32;
	LargeSize = 64;
	break;
    }

    do
    {
	int Size;
	long VPos = PPU.OBJ [S].VPos;
	
	if (PPU.OBJ [S].Size)
	    Size = LargeSize;
	else
	    Size = SmallSize;

	if (VPos >= PPU.screen_height)
	    VPos -= 256;
	    
	if (PPU.OBJ [S].HPos < 256 && PPU.OBJ [S].HPos > -Size &&
	    VPos < PPU.screen_height && VPos > -Size)
	{
	    VOBJ.OBJList [C++] = S;
	    VOBJ.Sizes[S] = Size;
	    VOBJ.VPositions[S] = VPos;
	}
	
	S = (S + 1) & 0x7f;
	
    } while (S != FirstSprite);

    // Terminate the list
    VOBJ.OBJList [C] = -1;
}

void DrawOBJS (void)
{
    dword Tx,Ty;
    dword BaseTile, Tile;
    int I,S;

    BG.BitShift = 4;
    BG.TileShift = 5;
    BG.TileAddress = PPU.OBJNameBase;
    BG.StartPalette = 128;
    BG.PaletteShift = 4;
    BG.PaletteMask = 7;
    BG.NameSelect = PPU.OBJNameSelect;
    BG.DirectColourMode = FALSE;
    BG.CacheBuffer = &TileCache.data [TILE_4BIT][0];
    BG.CacheValidation = &TileCache.valid [TILE_4BIT][0];

    I = 0;
    for (S = VOBJ.OBJList [I++]; S >= 0; S = VOBJ.OBJList [I++])
    {
	int VPos = VOBJ.VPositions [S];
	int Size = VOBJ.Sizes[S];
	int TileInc = 1;
	int Offset;

	if (VPos + Size <= (int) PPU.PreviousLine || VPos >= (int) PPU.CurrentLine )
	    continue;

	BaseTile = PPU.OBJ[S].Name | (PPU.OBJ[S].Palette << 10);

	if (PPU.OBJ[S].HFlip)
	{
	    BaseTile += ((Size >> 3) - 1) | H_FLIP;
	    TileInc = -1;
	}
	if (PPU.OBJ[S].VFlip)
	    BaseTile |= V_FLIP;
	    
	 BG.currentTileDepth = BG.zbuf_depth[PPU.OBJ[S].Priority];   

	{
	    int Left; 
	    int Right;
	    int X,Y;//loop variables
	    
	    //if (!GFX.pCurrentClip->Count [4])
	    //{
		Left = 0;
		Right = 256;
	    //}
	    //else
	    //{
	    //	Left = GFX.pCurrentClip->Left [clip][4];
	    //	Right = GFX.pCurrentClip->Right [clip][4];
	    //}

	    //if (Right <= Left || PPU.OBJ[S].HPos + Size <= Left ||
	    //	PPU.OBJ[S].HPos >= Right)
	    //	continue;
            
	    for (Y = 0; Y < Size; Y += 8)
	    {
		if (VPos + Y + 7 >= (int) PPU.PreviousLine && VPos + Y < (int) PPU.CurrentLine)
		{
		    int StartLine;
		    int TileLine;
		    int LineCount;
		    int Last;
		    int Middle;

		    if ((StartLine = VPos + Y) < (int) PPU.PreviousLine)
		    {
			StartLine = PPU.PreviousLine - StartLine;
			LineCount = 8 - StartLine;
		    }
		    else
		    {
			StartLine = 0;
			LineCount = 8;
		    }
		    if ((Last = VPos + Y + 8 - PPU.CurrentLine ) > 0)//remember that we have to render screen 
			if ((LineCount -= Last) <= 0)                       //untill line PPU.CurrentLine -1
			    break;

		    TileLine = StartLine << 3;
		    
		    Ty = (VPos + Y + StartLine) ;
    	  	   Tx = PPU.OBJ[S].HPos;
		    
		    if (!PPU.OBJ[S].VFlip)
			Tile = BaseTile + (Y << 1);
		    else
			Tile = BaseTile + ((Size - Y - 8) << 1);

		    Middle = Size >> 3;
		    if (PPU.OBJ[S].HPos < Left)
		    {      
		   	Tile += ((Left - PPU.OBJ[S].HPos) >> 3) * TileInc;
			Middle -= (Left - PPU.OBJ[S].HPos) >> 3;
			
			if ((Offset = (Left - PPU.OBJ[S].HPos) & 7))
			{
			    int W = 8 - Offset;
			    int Width = Right - Left;
			    if (W > Width)
				W = Width;

			    DrawTile (Tile ,  Tx ,  Ty, Offset, W, TileLine, LineCount );
			    
			    if (W >= Width)
				continue;
			    Tile += TileInc;
			    Middle--;
	    	  	   Tx += (8 - Offset);
			}
		    }
		   

		    if (PPU.OBJ[S].HPos + Size >= Right)
		    {
			Middle -= ((PPU.OBJ[S].HPos + Size + 7) -
				   Right) >> 3;
			Offset = (Right - (PPU.OBJ[S].HPos + Size)) & 7;
		    }
		    else
			Offset = 0;

		    for ( X = 0; X < Middle; X++, Tx += 8, Tile += TileInc)
		    {
			DrawTile (Tile ,  Tx ,  Ty, 0, 8, TileLine, LineCount );
		    }
		    if (Offset)
		    {
			DrawTile (Tile ,  Tx ,  Ty, 0, Offset, TileLine, LineCount );
		    }
		}
	    }
	}
    }
}


