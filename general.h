
#ifndef GENERAL_H
#define GENERAL_H

#include <stdio.h>
#include <stdlib.h>
              
#define TRUE	1
#define FALSE	0

#define true	TRUE
#define false	FALSE
/*
  restart the snes.
  the rom and the sram must already be loaded
*/
extern void reset_snes(void);

/*call this function to close the emulator*/                                          
extern void exit_snes(void);

		/*usefull type*/

typedef unsigned char byte; 		/*must be one byte long*/
typedef unsigned short int word;	/*must be two byte long*/
typedef unsigned long int dword;	/*must be four byte long*/
typedef unsigned char boolean;

		/*the above types signed*/

typedef signed char sbyte; 		/*must be one byte long*/
typedef signed short int sword;		/*must be two byte long*/
typedef signed long int sdword;		/*must be four byte long*/


#define READ_2BYTES(s) (*(word *) (s))


/* SNES screen width and height */
#define SNES_SCREEN_WIDTH		256
#define SNES_SCREEN_HEIGHT		224
#define SNES_SCREEN_HEIGHT_EXTENDED	239

#define SNES_NTSC_VCOUNTER_MAX  	262//from 0 to 261
#define SNES_PAL_VCOUNTER_MAX   	312//from 0 to 311
#define SNES_HCOUNTER_MAX		340//from 0 to 339 it seems that snes9x set it to 342

#define FIRST_VISIBLE_LINE		1

/*cpu frequency*/
#define FASTROM_SPEED			3579545//hz
#define SLOWROM_SPEED		    	2684659//hz

/*TV frequency*/
#define NTSC_TV_HZ			60
#define PAL_TV_HZ			50

extern struct settings{

  boolean is_ntsc;
  boolean is_fastrom;
  boolean is_hirom;
  int tv_hz;//60hz(ntsc) or 50hz(pal)
  int max_vcounter;
  int max_hcounter;

} snes_settings;

              
		/*header data found in the rom*/
extern struct headerdata {
	boolean hirom; // Apparent HiROM
	word hadd; // Set to 512 or 0
	char name [22];
	byte makeup;
	byte type;
	byte romsize; // Megabits
	byte sramsize; // Kilobits
	boolean fastrom;
	byte country, license, version;
	word resetvector, nmivector;
	word complement,checksum;
} *rom_header;


extern struct debug {
	boolean singleStep;
              boolean traceDMA;
              boolean traceRegs;
              boolean traceAudio;
              boolean traceExtra;
              boolean traceFPS;
} debugger;

                           
/************************************************************************
 *****************************OPTIONS**********************************
 ************************************************************************/
                 
#define DEBUGGER	
			/*turn off this definition to avoid (time consuming)
			 console messages and other things*/
                     
#ifdef DEBUGGER
	
	extern struct opcodeinfo {
		char *format; // The first three bytes MUST BE THE OPCODE.
		byte bytes, cycles;
	} opcodelist[];

	extern struct instructioninfo {
		char mnemonic[4];
		char *descr;
	} instrlist [] ;

	#define DEBUG(...)		if(debugger.traceExtra) printf(__VA_ARGS__)

#else
	#define DEBUG(...)

#endif

                                                                            
#define SOUND_SKIP
		/*
		   If defined, SOUND_SKIP enable some tricks for skipping game sound loop
		   that cause the game to execute an infinite loop and crash game
		*/


/*
 The definitions listed below are used to generate (maybe)fast code tourning
 off controls that may cause the emulator not to work properly.
*/

#define PFAST
		/*instead of using only one byte for processor status use
		a byte per flags so I can get the flags directly without
		masking the register.
		I don't know if it is fast because when the processor status register
		go on/down to the stack there is a little bit of work to do.
		But if there are more checking and setting then push and pull
		in this way we get performance improvement*/
		
//#define REGS_ALIGNED
		/*
		  this force the cpu emulated registers to be 32 bit aligned.
		  I don't know if it is usefull, maybe the compiler
		  do it by his own, but qwerty did it so i did it :)
		 */
//#define ALLOWROMWRITE
		/*
		 ROM Writes are normally prevented. But this cause
		 slowing down every memory write. Define this to
		 speed up the write. Make sure the game don't try
		 to write rom
		*/

/************************************************************************
********************Memory-mapped I/O optimizations:*********************
All memory accesses are supposed to be trapped for full emulation, but for
speed, many accesses can be NOT trapped.
So the emulator don't check if the address is in i/o registers space, but
suppose the address as a memory location. So the TRAPREAD and TRAPWRITE are
not called.
Here are a list of possible optimizations.
*************************************************************************/

#define FETCHFAST
		/*
		 opcode fetches are not intercepted. I suppose the opcode
		 isn't in the memory mapped i/o space
		*/
		
#define PUSHPULLFAST
		/*
		 Stack operations (PUSH and PULL) are not intercepted
		 (although stack relative operations are.)
		*/


//#define BLOCKMOVEFAST
		/*
		  block move instructions are not intercepted. I think
		  a program use DMA if it want access the memory mapped
		  registers
		 */

#endif /*end of library*/
