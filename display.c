
#include "SDL.h"
#include "general.h"
#include "display.h"
#include "ppu.h"


struct display { 

	SDL_Surface *screen;       //the screen surface
	
	Uint8* pixels;//pointer to frame buffer
	
	Uint8 * keyboard;   //  a snapshot of the current keyboard state

	void (*screenFilter)(void);//pointer to the actual screen filter function
		
	boolean fullscreen;//TRUE if fullscreen is setted now

	boolean strech;

              boolean scale;
	
	boolean increaseFrameskip;
	
	boolean decreaseFrameskip;

} DISPLAY;

#define MAX_ZOOM 3

struct display_setting SCREEN;

Uint32 convertedPalette[256];//this hold the snes palette converted to the actual screen format
Uint32 convertedRed[16][32];
Uint32 convertedGreen[16][32];
Uint32 convertedBlue[16][32];

#define getColor(paletteIndex)	convertedPalette[paletteIndex]

/*pointers to the actual renderTile function(the function used depends on the color depth)*/
void (*prenderTile)(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount);
void (*prenderTileVflip)(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount);
void (*prenderTileHflip)(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount);
void (*prenderTileHVflip)(byte* data, int paletteOffset ,int X, int Y,int StartPixel, int Width,int StartLine,int LineCount);
void (*pputpixelOffset)(dword offset, byte pixel);


void setRenderingFunctions(boolean fullScreen, boolean strech, int scale);

         /* Enable/Disable the events that have to be processed*/
int events_setup(void){

 /* disable "autofire"*/
 if( SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL) )
 {
 	fprintf( stderr, "Failed enabling key repeat\n");
  }

 /*Disable certain events processing.  Comments the lines corresponding to the
   event you want to be processed*/
 SDL_EventState(SDL_ACTIVEEVENT,SDL_IGNORE);		/* Application loses/gains visibility */
 //SDL_EventState(SDL_KEYDOWN,SDL_IGNORE);			/* Keys pressed */
 //SDL_EventState(SDL_KEYUP,SDL_IGNORE);			/* Keys released */
 //SDL_EventState(SDL_MOUSEMOTION,SDL_IGNORE);		/* Mouse moved */
 //SDL_EventState(SDL_MOUSEBUTTONDOWN,SDL_IGNORE);	/* Mouse button pressed */
 //SDL_EventState(SDL_MOUSEBUTTONUP,SDL_IGNORE);		/* Mouse button released */
 SDL_EventState(SDL_JOYAXISMOTION,SDL_IGNORE);		/* Joystick axis motion */
 SDL_EventState(SDL_JOYBALLMOTION,SDL_IGNORE);		/* Joystick trackball motion */
 SDL_EventState(SDL_JOYHATMOTION,SDL_IGNORE);		/* Joystick hat position change */
 SDL_EventState(SDL_JOYBUTTONDOWN,SDL_IGNORE);		/* Joystick button pressed */
 SDL_EventState(SDL_JOYBUTTONUP,SDL_IGNORE);		/* Joystick button released */
 SDL_EventState(SDL_QUIT,SDL_IGNORE);			/* User-requested quit */
 SDL_EventState(SDL_SYSWMEVENT,SDL_IGNORE);		/* System specific event */
 SDL_EventState(SDL_VIDEORESIZE,SDL_IGNORE);		/* User resized video mode */
 SDL_EventState(SDL_VIDEOEXPOSE,SDL_IGNORE);		/* Screen needs to be redrawn */

 DISPLAY.keyboard = SDL_GetKeyState( NULL );                    
 
 return 0;
}


void filterScreen1(void)
{
  int x,y;
  Uint8 *pix  = (Uint8 *)DISPLAY.pixels ;
  Uint8 *p;
  int bpp = DISPLAY.screen->format->BytesPerPixel;
  SDL_PixelFormat* fmt = DISPLAY.screen->format;
  
  if( SCREEN.zoom == 2)
  {
         for(y=0; y<SNES_SCREEN_HEIGHT_EXTENDED ; y++, pix+=SCREEN.bpl )
         {
  	 p = pix;                                                 
	  for(x=0; x< SNES_SCREEN_WIDTH; x++,p += SCREEN.bpp)   
	  {
	  	Uint32 c1,c2,c3;
  		Uint32 r1, g1, b1;
  		Uint32 r3, g3, b3;
                            
                            c1 = *(Uint16 *)p;
                            
                            r1 = (c1&fmt->Rmask);
                            g1 = (c1&fmt->Gmask);
                            b1 = (c1&fmt->Bmask);
                                                                                     
                            c2 = *(Uint16 *)(p+ SCREEN.bpp);
                                                        
              	r3 =  ((r1 + (c2&fmt->Rmask)) >>1) &fmt->Rmask;
              	g3 =  ((g1 + (c2&fmt->Gmask)) >>1) &fmt->Gmask;
              	b3 =  ((b1 + (c2&fmt->Bmask)) >>1) &fmt->Bmask;

		c3 = r3 | g3 | b3;
                            
		*(Uint16 *)(p+bpp) = (Uint16) c3;                            
		
		c2 = *(Uint16 *)(p+ SCREEN.bpl );
                                                        
              	r3 =  ((r1 + (c2&fmt->Rmask)) >>1) &fmt->Rmask;
              	g3 =  ((g1 + (c2&fmt->Gmask)) >>1) &fmt->Gmask;
              	b3 =  ((b1 + (c2&fmt->Bmask)) >>1) &fmt->Bmask;		
		c3 = r3 | g3 | b3;
                            
		*(Uint16 *)(p+DISPLAY.screen->pitch) = (Uint16) c3;                            
		
		c2 = *(Uint16 *)(p+ SCREEN.bpl+SCREEN.bpp );
                                                        
              	r3 =  ((r1 + (c2&fmt->Rmask)) >>1) &fmt->Rmask;
              	g3 =  ((g1 + (c2&fmt->Gmask)) >>1) &fmt->Gmask;
              	b3 =  ((b1 + (c2&fmt->Bmask)) >>1) &fmt->Bmask;
              	
		c3 = r3 | g3 | b3;
                            
		*(Uint16 *)(p+DISPLAY.screen->pitch+bpp) = (Uint16) c3;                            
	  	
	  }
        }
  }

  if( SCREEN.zoom == 3)
  {
         for(y=0; y<SNES_SCREEN_HEIGHT_EXTENDED ; y++, pix+=SCREEN.bpl )
         {
  	 p = pix;                                                 
	  for(x=0; x< SNES_SCREEN_WIDTH; x++,p += SCREEN.bpp)   
	  {
	  	Uint32 c1,c2,c3;
  		Uint8 r1, g1, b1;
  		Uint8 r2, g2, b2;
  		Uint8 sr1, sg1, sb1;
  		Uint8 sr2, sg2, sb2;
		Sint32 rdiff, gdiff, bdiff;
                            
                            c1 = *(Uint16 *)p;
                            
                            SDL_GetRGB( c1, DISPLAY.screen->format , &r1, &g1, &b1);

                            c2 = *(Uint16 *)(p+ SCREEN.bpp);

                            SDL_GetRGB( c2, DISPLAY.screen->format , &r2, &g2, &b2);

              	rdiff  =  ( (r1 - r2) / 3 ) ;
              	gdiff =  ( (g1 - g2) / 3 ) ;
              	bdiff =  ( (b1 - b2) / 3 ) ;

		sr1 = r2 += rdiff;
                            sg1 =g2 += gdiff;
                            sb1= b2 += bdiff;
                            
	              c3 = SDL_MapRGB(DISPLAY.screen->format, r2, g2, b2);

		*(Uint16 *)(p+bpp+bpp) = (Uint16) c3;                            

		r2 += rdiff;
                            g2 += gdiff;
                            b2 += bdiff;
                            
	              c3 = SDL_MapRGB(DISPLAY.screen->format, r2, g2, b2);
                            
		*(Uint16 *)(p+bpp) = (Uint16) c3;                            

		
		c2 = *(Uint16 *)(p+ SCREEN.bpl );

                            SDL_GetRGB( c2, DISPLAY.screen->format , &r2, &g2, &b2);

              	rdiff  =  ( (r1 - r2) / 3 ) ;
              	gdiff =  ( (g1 - g2) / 3 ) ;
              	bdiff =  ( (b1 - b2) / 3 ) ;

		sr2 = r2 += rdiff;
                            sg2 =g2 += gdiff;
                            sb2= b2 += bdiff;
                            
	              c3 = SDL_MapRGB(DISPLAY.screen->format, r2, g2, b2);
                            
		*(Uint16 *)(p+DISPLAY.screen->pitch+DISPLAY.screen->pitch) =   (Uint16) c3;                            

		r2 += rdiff;
                            g2 += gdiff;
                            b2 += bdiff;
                              
	              c3 = SDL_MapRGB(DISPLAY.screen->format, r2, g2, b2);
                            
		*(Uint16 *)(p+DISPLAY.screen->pitch) = (Uint16) c3;                            

		
		c2 = *(Uint16 *)(p+ SCREEN.bpl+SCREEN.bpp );

                            SDL_GetRGB( c2, DISPLAY.screen->format , &r2, &g2, &b2);

              	rdiff  =  ( (r1 - r2) / 3 ) ;
              	gdiff =  ( (g1 - g2) / 3 ) ;
              	bdiff =  ( (b1 - b2) / 3 ) ;

		r2 += rdiff;
                            g2 += gdiff;
                            b2 += bdiff;
                            
	              c3 = SDL_MapRGB(DISPLAY.screen->format, r2, g2, b2);
                            
		*(Uint16 *)(p+DISPLAY.screen->pitch+DISPLAY.screen->pitch+bpp+bpp) = (Uint16)c3;

		*(Uint16 *)(p+DISPLAY.screen->pitch+DISPLAY.screen->pitch+bpp) = 
			(Uint16)SDL_MapRGB(DISPLAY.screen->format, (r2+sr2)>>1, (g2+sg2)>>1, (b2+sb2)>>1 );

		*(Uint16 *)(p+DISPLAY.screen->pitch+bpp+bpp) =
			(Uint16)SDL_MapRGB(DISPLAY.screen->format, (r2+sr1)>>1, (g2+sg1)>>1, (b2+sb1)>>1 );

		r2 += rdiff;
                            g2 += gdiff;
                            b2 += bdiff;
                            
	              c3 = SDL_MapRGB(DISPLAY.screen->format, r2, g2, b2);

		*(Uint16 *)(p+DISPLAY.screen->pitch+bpp) = (Uint16) c3;                            
	  	
	  }
        }
  }

}

void filterScreen2(void)
{
  int x,y;
  Uint8 *pix  = (Uint8 *)DISPLAY.pixels ;
  Uint8 *p;
  int bpp = DISPLAY.screen->format->BytesPerPixel;
     
  if( SCREEN.zoom == 2)
  { 
       for(y=0; y<SNES_SCREEN_HEIGHT_EXTENDED ; y++, pix+=SCREEN.bpl )
       {
  	 p = pix;                                                 
	  for(x=0; x< SNES_SCREEN_WIDTH; x++, p += SCREEN.bpp)   
	  {
	  	Uint32 c1,c2;
                           
                            c1 = *(Uint16 *)(p-SCREEN.bpl);
                           
                            c2 = *(Uint16 *)(p+SCREEN.bpp );
                            
                            if( c1 == c2 )
			*(Uint16 *)(p+bpp) =(Uint16)c1;                                                      
		else
			*(Uint16 *)(p+bpp) =*(Uint16 *)p;
			
			
                            c1 = *(Uint16 *)(p+SCREEN.bpl);

                            if( c1 == c2 )
			*(Uint16 *)(p+bpp+DISPLAY.screen->pitch) = (Uint16)c1;                                                      
		else
			*(Uint16 *)(p+bpp+DISPLAY.screen->pitch) =*(Uint16 *)p;
			
			
                            c2 = *(Uint16 *)(p-SCREEN.bpp );                            

                            if( c1 == c2 )
			*(Uint16 *)(p+DISPLAY.screen->pitch) = (Uint16)c1;                                                      
		else
			*(Uint16 *)(p+DISPLAY.screen->pitch) =*(Uint16 *)p;
	  	
	  }
       }
  }

  if( SCREEN.zoom == 3)
  { 
       for(y=0; y<SNES_SCREEN_HEIGHT_EXTENDED ; y++, pix+=SCREEN.bpl )
       {
  	 p = pix;                                                 
	  for(x=0; x< SNES_SCREEN_WIDTH; x++, p += SCREEN.bpp)   
	  {
	  	Uint32 c1,c2;

		*(Uint16 *)(p+bpp+DISPLAY.screen->pitch) = *(Uint16*) (p);
                           
                            c1 = *(Uint16 *)(p-SCREEN.bpl+bpp+DISPLAY.screen->pitch);
                           
                            c2 = *(Uint16 *)(p+SCREEN.bpp );
                            
                            if( c1 == c2 )
		{
			*(Uint16 *)(p+bpp+bpp) = (Uint16)c1;
		}
		else
		{
			*(Uint16 *)(p+bpp+bpp) = *(Uint16 *)p;
		}

		*(Uint16 *)(p+bpp) =
		*(Uint16 *)(p+bpp+bpp+DISPLAY.screen->pitch) = *(Uint16 *)p;
			

                            c1 = *(Uint16 *)(p+SCREEN.bpl);

                            if( c1 == c2 )
		{
			*(Uint16 *)(p+bpp+bpp+DISPLAY.screen->pitch+DISPLAY.screen->pitch) = (Uint16)c1;                                                      
		}
		else
		{
			*(Uint16 *)(p+bpp+bpp+DISPLAY.screen->pitch+DISPLAY.screen->pitch) =*(Uint16 *)p;
		}

		*(Uint16 *)(p+bpp+DISPLAY.screen->pitch+DISPLAY.screen->pitch) = *(Uint16 *)p;
		
			
                            c2 = *(Uint16 *)(p-SCREEN.bpp+bpp+DISPLAY.screen->pitch );                            

                            if( c1 == c2 )
		{
			*(Uint16 *)(p+DISPLAY.screen->pitch+DISPLAY.screen->pitch) = (Uint16)c1;                                                      
		}
		else
		{
			*(Uint16 *)(p+DISPLAY.screen->pitch+DISPLAY.screen->pitch) =*(Uint16 *)p;
		}                                         

		*(Uint16 *)(p+DISPLAY.screen->pitch ) = *(Uint16 *)p;
                                                                              
                            c1 = *(Uint16 *)(p-SCREEN.bpl+bpp+DISPLAY.screen->pitch);
                              
                            if( c1 == c2 )
		{
			*(Uint16 *)(p) =  (Uint16)c1;                                                      
		}
                                                                           
                                                                    
	  }
       }
  }
}


void filterScreen3(void)
{
  int x,y;
  Uint8 *pix  = (Uint8 *)DISPLAY.pixels ;
  Uint8 *p;
  int bpp = DISPLAY.screen->format->BytesPerPixel;

  if( SCREEN.zoom == 2)
  {
         for(y=0; y<SNES_SCREEN_HEIGHT_EXTENDED ; y++, pix+=SCREEN.bpl )
         {
  	 p = pix;
	  for(x=0; x< SNES_SCREEN_WIDTH; x++,p += SCREEN.bpp)
	  {
	  	Uint32 c1;

		c1 = *(Uint16 *)p;

		*(Uint16 *)(p+bpp) = (Uint16) c1;
		
		*(Uint16 *)(p+DISPLAY.screen->pitch) = (Uint16) c1;
		
		*(Uint16 *)(p+DISPLAY.screen->pitch+bpp) = (Uint16) c1;


	  }
         }
  }

  if( SCREEN.zoom == 3)
  {
         for(y=0; y<SNES_SCREEN_HEIGHT_EXTENDED ; y++, pix+=SCREEN.bpl )
         {
  	 p = pix;
	  for(x=0; x< SNES_SCREEN_WIDTH; x++,p += SCREEN.bpp)
	  {
	  	Uint32 c1;

		c1 = *(Uint16 *)p;

		*(Uint16 *)(p+bpp) = (Uint16) c1;

		*(Uint16 *)(p+bpp+bpp) = (Uint16) c1;
		
		*(Uint16 *)(p+DISPLAY.screen->pitch) = (Uint16) c1;
		
		*(Uint16 *)(p+DISPLAY.screen->pitch+bpp) = (Uint16) c1;

		*(Uint16 *)(p+DISPLAY.screen->pitch+bpp+bpp) = (Uint16) c1;

		*(Uint16 *)(p+DISPLAY.screen->pitch+DISPLAY.screen->pitch) = (Uint16) c1;
		
		*(Uint16 *)(p+DISPLAY.screen->pitch+DISPLAY.screen->pitch+bpp) = (Uint16) c1;

		*(Uint16 *)(p+DISPLAY.screen->pitch+DISPLAY.screen->pitch+bpp+bpp) = (Uint16) c1;


	  }
         }
  }

}

void filterScreen4(void)
{
  int x,y;
  Uint8 *pix  = (Uint8 *)DISPLAY.pixels ;
  Uint8 *p;
  int bpp = DISPLAY.screen->format->BytesPerPixel;
  SDL_PixelFormat* fmt = DISPLAY.screen->format;
  
  if( SCREEN.zoom == 2)
  {
         for(y=0; y<SNES_SCREEN_HEIGHT_EXTENDED ; y++, pix+=SCREEN.bpl )
         {
  	 p = pix;                                                 
	  for(x=0; x< SNES_SCREEN_WIDTH; x++,p += SCREEN.bpp)   
	  {
	  	Uint32 c1,c2,c3;
  		Uint32 r1, g1, b1;
  		Uint32 r3, g3, b3;
                            
                            c1 = *(Uint16 *)p;
                            
                            r1 = (c1&fmt->Rmask);
                            g1 = (c1&fmt->Gmask);
                            b1 = (c1&fmt->Bmask);
                                                                                     
                            c2 = *(Uint16 *)(p+ SCREEN.bpp);
                                                        
              	r3 =  ((r1 + (c2&fmt->Rmask)) >>1) &fmt->Rmask;
              	g3 =  ((g1 + (c2&fmt->Gmask)) >>1) &fmt->Gmask;
              	b3 =  ((b1 + (c2&fmt->Bmask)) >>1) &fmt->Bmask;

		c3 = r3 | g3 | b3;
                            
		*(Uint16 *)(p+bpp) = (Uint16) c3;                            
		
		c2 = *(Uint16 *)(p+ SCREEN.bpl );
                                                        
              	r3 =  ((r1 + (c2&fmt->Rmask)) >>1) &fmt->Rmask;
              	g3 =  ((g1 + (c2&fmt->Gmask)) >>1) &fmt->Gmask;
              	b3 =  ((b1 + (c2&fmt->Bmask)) >>1) &fmt->Bmask;		
		c3 = r3 | g3 | b3;
                            
		*(Uint16 *)(p+DISPLAY.screen->pitch) = (Uint16) c3;                            
		
		c2 = *(Uint16 *)(p+ SCREEN.bpl+SCREEN.bpp );
                                                        
              	r3 =  ((r1 + (c2&fmt->Rmask)) >>1) &fmt->Rmask;
              	g3 =  ((g1 + (c2&fmt->Gmask)) >>1) &fmt->Gmask;
              	b3 =  ((b1 + (c2&fmt->Bmask)) >>1) &fmt->Bmask;
              	
		c3 = r3 | g3 | b3;
                            
		*(Uint16 *)(p+DISPLAY.screen->pitch+bpp) = (Uint16) c3;                            
	  	
	  }
        }
  }

  if( SCREEN.zoom == 3)
  {
         for(y=0; y<SNES_SCREEN_HEIGHT_EXTENDED ; y++, pix+=SCREEN.bpl )
         {
  	 p = pix;                                                 
	  for(x=0; x< SNES_SCREEN_WIDTH; x++,p += SCREEN.bpp)   
	  {
	  	Uint32 c1,c2,c3;
  		Uint32 r1, g1, b1;
  		Uint32 r3, g3, b3;
                            
                            c1 = *(Uint16 *)p;

		*(Uint16 *)(p+bpp) =
		*(Uint16 *)(p+bpp+DISPLAY.screen->pitch) =
		*(Uint16 *)(p+DISPLAY.screen->pitch) = 	c1;

                            r1 = (c1&fmt->Rmask);
                            g1 = (c1&fmt->Gmask);
                            b1 = (c1&fmt->Bmask);
                                                                                     
                            c2 = *(Uint16 *)(p+ SCREEN.bpp);
                                                        
              	r3 =  ((r1 + (c2&fmt->Rmask)) >>1) &fmt->Rmask;
              	g3 =  ((g1 + (c2&fmt->Gmask)) >>1) &fmt->Gmask;
              	b3 =  ((b1 + (c2&fmt->Bmask)) >>1) &fmt->Bmask;

		c3 = r3 | g3 | b3;
                            
		*(Uint16 *)(p+bpp+bpp) = 
		*(Uint16 *)(p+bpp+bpp+DISPLAY.screen->pitch) = (Uint16) c3;                            
		
		c2 = *(Uint16 *)(p+ SCREEN.bpl );
                                                        
              	r3 =  ((r1 + (c2&fmt->Rmask)) >>1) &fmt->Rmask;
              	g3 =  ((g1 + (c2&fmt->Gmask)) >>1) &fmt->Gmask;
              	b3 =  ((b1 + (c2&fmt->Bmask)) >>1) &fmt->Bmask;		

		c3 = r3 | g3 | b3;

		*(Uint16 *)(p+DISPLAY.screen->pitch+DISPLAY.screen->pitch+bpp) =                            
		*(Uint16 *)(p+DISPLAY.screen->pitch+DISPLAY.screen->pitch) = (Uint16) c3;                            
		
		c2 = *(Uint16 *)(p+ SCREEN.bpl+SCREEN.bpp );
                                                        
              	r3 =  ((r1 + (c2&fmt->Rmask)) >>1) &fmt->Rmask;
              	g3 =  ((g1 + (c2&fmt->Gmask)) >>1) &fmt->Gmask;
              	b3 =  ((b1 + (c2&fmt->Bmask)) >>1) &fmt->Bmask;
              	
		c3 = r3 | g3 | b3;
                            
		*(Uint16 *)(p+DISPLAY.screen->pitch+DISPLAY.screen->pitch+bpp+bpp) = (Uint16) c3;                            
	  	
	  }
        }

  }

}


void setFilter(int n)
{

	switch(n)
	{
              	case 1:   DISPLAY.screenFilter = filterScreen3; break;
		case 2:   DISPLAY.screenFilter = filterScreen2; break;
		case 3: 	 DISPLAY.screenFilter = filterScreen1; break;
		case 4: 	 DISPLAY.screenFilter = filterScreen4; break;
	}

}

void handle_input(void)
{
	 SDL_PumpEvents();
	 
	/*this part is related to emulator commands, not snes issue*/              	 
             	if( DISPLAY.keyboard [ SDLK_ESCAPE ] )  //when 'esc' is pressed exit emulator
             	{
#ifdef DEBUGGER
		DISPLAY.fullscreen = FALSE;
		setRenderingFunctions(  DISPLAY.fullscreen , DISPLAY.strech, DISPLAY.scale );
		debugger.singleStep = TRUE;
#else
            		exit_snes();
#endif
             	}
             		
            	else if(  DISPLAY.keyboard [ SDLK_F1 ] )   //toggle fullscreen
            	{
            	     if(DISPLAY.fullscreen)
            		DISPLAY.fullscreen = FALSE;				             	     
            	     else
            		DISPLAY.fullscreen = TRUE;

          	    setRenderingFunctions(  DISPLAY.fullscreen , DISPLAY.strech, DISPLAY.scale );
            	}
            		
            	else if( DISPLAY.keyboard [ SDLK_F2 ] )   //restart snes
            	{
#ifdef DEBUGGER
            		DISPLAY.fullscreen = FALSE;
		setRenderingFunctions(  DISPLAY.fullscreen , DISPLAY.strech, DISPLAY.scale );
#endif
		reset_snes();		 
	}
		
            	else if( DISPLAY.keyboard [ SDLK_F3 ] ) //strech screen
            	{
            	     if(DISPLAY.strech)
            		DISPLAY.strech = FALSE;				             	     
            	     else
            		DISPLAY.strech = TRUE;

	     setRenderingFunctions(  DISPLAY.fullscreen , DISPLAY.strech, DISPLAY.scale );
            	}            	
            	            	
	//else if( DISPLAY.keyboard [ SDLK_F4 ] )   //change screen filter
            	
			/*screen filters*/            	 
	else if( DISPLAY.keyboard [ SDLK_F5 ] )   //change screen filter
                             setFilter(1);
            		 
            	else if( DISPLAY.keyboard [ SDLK_F6 ] )   //change screen filter
                             setFilter(2);
            		 
            	else if( DISPLAY.keyboard [ SDLK_F7 ] )   //change screen filter
                             setFilter(3);

            	else if( DISPLAY.keyboard [ SDLK_F8 ] )   //change screen filter
                             setFilter(4);

           	else if( DISPLAY.keyboard [ SDLK_F9 ] )   //change window size
	{
		DISPLAY.scale--;
		if( DISPLAY.scale < 1) DISPLAY.scale = 1;
		setRenderingFunctions(  DISPLAY.fullscreen , DISPLAY.strech, DISPLAY.scale );
	}

           	else if( DISPLAY.keyboard [ SDLK_F10 ] ) //change window size
	{
		DISPLAY.scale++;
		if( DISPLAY.scale > MAX_ZOOM ) DISPLAY.scale = MAX_ZOOM;
		setRenderingFunctions(  DISPLAY.fullscreen , DISPLAY.strech, DISPLAY.scale );
	}
            		            	
            	else if( DISPLAY.keyboard [ SDLK_KP_PLUS ] )   //increase frameskip
            	{
            		DISPLAY.increaseFrameskip = TRUE;
            	}            	
            		
            	else if( DISPLAY.keyboard [ SDLK_KP_MINUS ] )   //decrease frameskip
            	{
            		DISPLAY.decreaseFrameskip = TRUE;
            	}


}


/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/

//call this at the beginnig of the frame. 
void syncSpeed(void)
{
	static Uint32 T0 = 0;   
	static Uint32 delay = 0;

   	Uint32 t = SDL_GetTicks();
/*	printf("\n%d frame skip, t %d,  T0 %d,  t - T0 %d", PPU.frame_skip,t,T0, t -T0);
	if ((t - T0) >= 1000 )
	{
		if(){
			double seconds = (t - T1) / 1000.0;
		      	DISPLAY.decreaseFrameskip = TRUE;
			T0 = t;	
		}
		else
		{
			DISPLAY.increaseFrameskip = TRUE;
		}
             		SDL_Delay( delay );
	}
   */
#ifdef DEBUGGER
	{
	static Uint32 T1 = 0;   
	static int Frames = 0;
	if (t - T1 >= 5000)
	{
	    double seconds = (t - T1) / 1000.0;
	    double fps;
	    Frames = PPU.frame_number - Frames;	
	    fps = Frames / seconds;
	    if(  debugger.traceFPS )
	    {
	    	printf("%d frames with %d frame skip in %g seconds = %g FPS\n", Frames,PPU.frame_skip, seconds, fps);
	    }
	    T1 = t;
	    Frames = PPU.frame_number;
	}
	}
#endif

	if( DISPLAY.increaseFrameskip == TRUE )   //increase frameskip
  	{
  		DISPLAY.increaseFrameskip = FALSE;
	  	PPU.frame_skip++;
	  	if ( PPU.frame_skip > 60 )  PPU.frame_skip = 60;
	}            	
	else if( DISPLAY.decreaseFrameskip == TRUE )   //decrease frameskip
	{
		DISPLAY.decreaseFrameskip = FALSE;
		PPU.frame_skip--;
		if ( PPU.frame_skip < 1 )  PPU.frame_skip = 1;
	}            		         		
   
}

void resetRGB(void)
{
 Uint8 r,g,b;
 int brightness;
 
 for( brightness = 0; brightness < 16; brightness++ )
 {
	 for(r=0;r<32;r++)
	 {
		convertedRed[brightness][r] =SDL_MapRGB( DISPLAY.screen->format,  
	           					/*   red  */   ((r<<3) * brightness) / 0xf ,
	           					/* green*/   0 ,
	           					/*  blue */    0 );
	 }
  }
  
 for( brightness = 0; brightness < 16; brightness++ )	 	 
 {
	 for(g=0;g<32;g++)
	 {
		convertedGreen[brightness][g] =SDL_MapRGB( DISPLAY.screen->format,  
	           					/*   red  */   0 ,
	           					/* green*/   ((g<<3)* brightness) / 0xf ,
	           					/*  blue */    0 );
	 }
  } 
	 
  for( brightness = 0; brightness < 16; brightness++ )
  {
	 for(b=0;b<32;b++)
	 {
		convertedBlue[brightness][b] =SDL_MapRGB( DISPLAY.screen->format,  
	           					/*   red  */   0 ,
	           					/* green*/   0 ,
	           					/*  blue */    ((b<<3)* brightness) / 0xf );
	 }
   }

}

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
	           		convertedPalette[color] =     convertedRed [0xF] [ ( c  & 0x001F )                 ]  |
	           					convertedGreen [0xF] [ ( ( c  & 0x03E0 )  >>   5 ) ]  |
	           					   convertedBlue [0xF] [ ( ( c  & 0x7C00 )  >> 10 ) ];
		}
	}
	else
	{
		for( color = 0; color < 256; color++)
		{
                             	word c = snes_palette [color];
	           		convertedPalette[color] =     convertedRed [PPU.Brightness] [ ( c  & 0x001F )                 ]  |
	           					convertedGreen [PPU.Brightness] [ ( ( c  & 0x03E0 )  >>   5 ) ]  |
	           					   convertedBlue [PPU.Brightness] [ ( ( c  & 0x7C00 )  >> 10 ) ];

		}	
	}
}

/*
 call this function when one color in the snes palette is changed
*/
void UpdateDisplayPaletteColor( word color, int colorNumber )
{

           	convertedPalette[colorNumber] =     convertedRed [PPU.Brightness] [ ( color  & 0x001F )                 ]  |
					convertedGreen [PPU.Brightness] [ ( ( color  & 0x03E0 )  >>   5 ) ]  |
					   convertedBlue [PPU.Brightness] [ ( ( color  & 0x7C00 )  >> 10 ) ];

}



/*Call this before the rendering of the current frame*/
void clearScreen(void)
{
  if( !(PPU.frame_number % 6)) handle_input();//every 6 frame handle emultor's input
  
  SDL_FillRect( DISPLAY.screen, NULL, convertedPalette[0]);//clean the entire screen
  
  SDL_LockSurface(DISPLAY.screen);//lock surface in order to access frame buffer
}

/*
 when the snes screen is ready to be drawn call this function
*/
void renderSnesScreen(void)
{

  if(  DISPLAY.screenFilter!= NULL) (* DISPLAY.screenFilter)();//when the resolution is greater than snes resolution
  							   //a filter could be used to achieve a good quality image
  
  SDL_UnlockSurface( DISPLAY.screen);//unlock surface(the surface was locked by clearscreen function)
  
  SDL_Flip( DISPLAY.screen);//swap the hiden buffer with screen buffer
         
}


 /*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */

void putpixel1Offset(dword offset, byte pixel)
{
     *(Uint8 *)((Uint8 *)DISPLAY.pixels + offset) =   getColor(pixel);
}

void putpixel2Offset(dword offset, byte pixel)
{
     *(Uint16 *)((Uint8 *)DISPLAY.pixels + offset) =   getColor(pixel);
}

void putpixel3Offset(dword offset, byte pixel)
{
    // Here p is the address to the pixel we want to set 
    Uint8 *p= (Uint8 *)DISPLAY.pixels + offset;
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

void putpixel4Offset(dword offset, byte pixel)
{
     *(Uint32 *)((Uint8 *)DISPLAY.pixels + offset) =   getColor(pixel);
}

              
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
 Uint8 *p = (Uint8 *)DISPLAY.pixels;\
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
 Uint8 *p = (Uint8 *)DISPLAY.pixels;\
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
 Uint8 *p = (Uint8 *)DISPLAY.pixels;\
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
 Uint8 *p = (Uint8 *)DISPLAY.pixels;\
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



/*
  call this function when the snes need to know the joypad status
  Also handle emulator exit
*/
  
#define SNES_TR_MASK	    	0x10
#define SNES_TL_MASK	    	0x20
#define SNES_X_MASK	    	0x40
#define SNES_A_MASK	    	0x80
#define SNES_RIGHT_MASK	0x100
#define SNES_LEFT_MASK	0x200
#define SNES_DOWN_MASK	0x400
#define SNES_UP_MASK	    	0x800
#define SNES_START_MASK	0x1000
#define SNES_SELECT_MASK      0x2000
#define SNES_Y_MASK	              0x4000
#define SNES_B_MASK	   	0x8000
 
 /*chose the map for the snes joypad with the keybord input*/
struct {

             SDLKey up;
             SDLKey down;
             SDLKey left;
             SDLKey right;
             SDLKey a;
             SDLKey b;
             SDLKey x;
             SDLKey y;
             SDLKey l;
             SDLKey r;
             SDLKey select;
             SDLKey start;
       
} padkey [] ={ //P1 keys
	      { SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, SDLK_a, SDLK_s, SDLK_x,
                    SDLK_z, SDLK_d,SDLK_c, SDLK_SPACE, SDLK_RETURN },
                    //P2 keys
	      { SDLK_KP8, SDLK_KP2, SDLK_KP4, SDLK_KP6, SDLK_f, SDLK_g, SDLK_b,
                    SDLK_v, SDLK_h,SDLK_n, SDLK_KP0, SDLK_KP1 }
                    };

word getjoypad(int joypad){

              word pad ;

              if(joypad == 0) //here I assume that when  this function is called the first joypad read will be 0 
              	 SDL_PumpEvents();
              pad = 0;	 

	if( DISPLAY.keyboard [ padkey [joypad].up ] )  	pad |= SNES_UP_MASK; 
             	if( DISPLAY.keyboard [ padkey [joypad].down ] ) 	pad |= SNES_DOWN_MASK; 
             	if( DISPLAY.keyboard [ padkey [joypad].left ] )  	pad |= SNES_LEFT_MASK; 
             	if( DISPLAY.keyboard [ padkey [joypad].right ] )  	pad |= SNES_RIGHT_MASK; 
          	if( DISPLAY.keyboard [ padkey [joypad].a ] )  	pad |= SNES_A_MASK; 
             	if( DISPLAY.keyboard [ padkey [joypad].b ] ) 	pad |= SNES_B_MASK; 
             	if( DISPLAY.keyboard [ padkey [joypad].x ] ) 	pad |= SNES_X_MASK; 
             	if( DISPLAY.keyboard [ padkey [joypad].y ] )		pad |= SNES_Y_MASK; 
	if( DISPLAY.keyboard [ padkey [joypad].l ] ) 	 	pad |= SNES_TL_MASK; 
             	if( DISPLAY.keyboard [ padkey [joypad].r ] ) 		pad |= SNES_TR_MASK; 
             	if( DISPLAY.keyboard [ padkey [joypad].select ] ) 	pad |= SNES_SELECT_MASK; 
             	if( DISPLAY.keyboard [ padkey [joypad].start ] ) 	pad |= SNES_START_MASK; 

             	//it's not possible to press up and down in the same time. The same for left and right
              if ( pad & SNES_LEFT_MASK)
			pad &= ~SNES_RIGHT_MASK;
	if ( pad & SNES_UP_MASK)
			pad &= ~SNES_DOWN_MASK;
               
 	return pad;
}

void setRenderingFunctions(boolean fullScreen, boolean strech, int scale)
{
   
  if( fullScreen )
  {
   	   DISPLAY.screen = SDL_SetVideoMode  ( 1024, 768,  16, 
				 ( SDL_DOUBLEBUF | SDL_HWSURFACE | SDL_FULLSCREEN ) );
  }
  else
  {
      	DISPLAY.screen = SDL_SetVideoMode  ( (SNES_SCREEN_WIDTH+2)*DISPLAY.scale, 
						(SNES_SCREEN_HEIGHT_EXTENDED+2)*DISPLAY.scale,  16, 
				 ( SDL_DOUBLEBUF | SDL_HWSURFACE ) );
   }
   
   if (DISPLAY.screen == NULL) 
   {
	printf("Unable to set video mode: %s\n", SDL_GetError());
	exit_snes();
   }
   
  SCREEN.zoom = DISPLAY.screen->w /  SNES_SCREEN_WIDTH;
  if( (DISPLAY.screen->h / SNES_SCREEN_HEIGHT_EXTENDED) < SCREEN.zoom)
                           SCREEN.zoom = DISPLAY.screen->h / SNES_SCREEN_HEIGHT_EXTENDED;

  SCREEN.bpl = SCREEN.zoom * DISPLAY.screen->pitch;
  SCREEN.bpp = SCREEN.zoom * DISPLAY.screen->format->BytesPerPixel;

  if( SCREEN.zoom > MAX_ZOOM ) SCREEN.zoom = MAX_ZOOM;
  
  DISPLAY.pixels = DISPLAY.screen->pixels +  		 
  			( ( ( DISPLAY.screen->w - ( SCREEN.zoom * SNES_SCREEN_WIDTH) ) / 2) * 
  			  DISPLAY.screen->format->BytesPerPixel)+
  			( ( ( DISPLAY.screen->h - ( SCREEN.zoom * SNES_SCREEN_HEIGHT_EXTENDED ) ) / 2)  *
  			 DISPLAY.screen->pitch );
  
  switch(DISPLAY.screen->format->BytesPerPixel)
  {
  	case 1:   prenderTile = renderTile1;
  	  	prenderTileVflip = renderTile1Vflip;
  		prenderTileHflip = renderTile1Hflip;
  		prenderTileHVflip = renderTile1HVflip;
  		//pputpixel = putpixel1;
  		pputpixelOffset = putpixel1Offset;
  		break;
  		
  	case 2:  prenderTile = renderTile2;
  		prenderTileVflip = renderTile2Vflip;
  		prenderTileHflip = renderTile2Hflip;
  		prenderTileHVflip = renderTile2HVflip;
  		//pputpixel = putpixel2;
  		pputpixelOffset = putpixel2Offset;	
  		break;
  		
  	case 3:   prenderTile = renderTile3;
  	  	prenderTileVflip = renderTile3Vflip;
  		prenderTileHflip = renderTile3Hflip;
  		prenderTileHVflip = renderTile3HVflip;
  		//pputpixel = putpixel3;
  		pputpixelOffset = putpixel3Offset;
  		break;          
  		
  	case 4:   prenderTile = renderTile4;
  	  	prenderTileVflip = renderTile4Vflip;
  		prenderTileHflip = renderTile4Hflip;
  		prenderTileHVflip = renderTile4HVflip;
  		//pputpixel = putpixel4;
  		pputpixelOffset = putpixel4Offset;
  		break;
  }

}

/**************************************************
  Initialize the Video and the input
  return value:
	0 if ok
            -1 if an error occur
***************************************************/
int initVideo(void)
{
  SDL_Surface *icon;   
                          
        /* initialize SDL */
  if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER  ) < 0 )
    {
      fprintf( stderr, "SDL initialization failed: %s\n", SDL_GetError( ) );
      return -1;
    }

  if( atexit(SDL_Quit) ){
      fprintf( stderr, "Failed SDL cleaning function set up (atexit(SDL_Quit)) and ignoring problem :)\n");
  }
    
      /* Imposta titolo finestra ed icona */
  icon = SDL_LoadBMP( ICON_NAME );
  if(icon == NULL)
  {
    fprintf( stderr, "Icon loading failed: %s\n", SDL_GetError( ) );
  } 
  else SDL_WM_SetIcon(icon, NULL);

  SDL_WM_SetCaption(rom_header->name , WINDOW_TITLE );
  
  /*setting up input handling*/  
  events_setup();

  DISPLAY.increaseFrameskip = FALSE;
  DISPLAY.decreaseFrameskip = FALSE;
  DISPLAY.screenFilter = NULL;
  DISPLAY.fullscreen = FALSE;
  DISPLAY.strech = FALSE;
  DISPLAY.scale = 2;

  setRenderingFunctions(  DISPLAY.fullscreen , DISPLAY.strech, DISPLAY.scale );

  setFilter(1);

  resetRGB();//you have to call this function after the screen set up
                                                                                  
  return 0;
}  



