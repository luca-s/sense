/*the purpose of this file is the memory management.
  So there are the functions to map the snes address to the appropriate
  memory and to manage the memory mapped I/O registers.*/

#include <string.h>
#include "general.h"
#include "dma.h"
#include "cpu.h"
#include "mmio.h"
#include "ppu.h"
#include "display.h"
#include "tileConverter.h"


/************************************************************************/
/***********************snes memory definitions******************************/
/************************************************************************/

byte snes_memory[ 0x20000 + //cpu ram 128kB
		   0x8000 + //sram 256kbits
		  0x10000 + //expantion ram ??? It is used only with the old memory map
		 0x600000 + //rom(is enough memory to load the biggest rom suppported)
		   0x4000 ];//registers

/*
 Ok, some explanations.
 The mempry is defined in this way to get performance improvement.
 In mmio.h the memory are defined in this way:

#define ram 	(byte* snes_memory)				cpu ram
#define sram 	(byte* snes_memory + 0x20000)			rom back up memory
#define expram	(byte* snes_memory + 0x28000)			expantion ram
#define rom	(byte* snes_memory +  0x48000 + 512 + 544)	the game code & data
#define registers	(byte* snes_memory + 0x648000 + 512 + 544)	memory mapped i/o registers

 After using SNESMEM  we can control if a trapregwrite should be performed
 only checking this:

 if (ptr>=registers) //(ptr is the pointer returned from SNESMEM)
     trapregwrite();

 If all kind of memory wasn't continuous the check should be:

  if(ptr>=registers && ptr<=registers+0x4000)
  	trapregwrite();
*/

#ifndef ALLOWROMWRITE
byte backup_rom[0x600000];
#endif

void set_mapping_table_addresses  (boolean hirom) ;

void resetmemory(){

	memset (ram      , 0, 0x20000 );
	memset (expram   , 0, 0x10000 );
	memset (registers, 0, 0x4000);
	#ifndef ALLOWROMWRITE
		memcpy(backup_rom,rom,0x600000);/*if a game write in rom space I use the backup
		                data to restore the original value before continue execution*/
	#endif

	set_mapping_table_addresses(snes_settings.is_hirom);
}


/***********************************************************************/
/***********************SNES MEMORY MAPPING*****************************/
/***********************************************************************/

byte *startaddr [0x800];
    /*  For quick address aaccess.  To access a 65c816 address bb.aaaa, stored in
	variable addr:
		startaddr [addr >> 13] + (addr)
		This array is filled by getstartaddresses ().
	*/

#define AA_NOTHING 	expram

struct addressarea {
	// this is a representation of the SNES memory map. It's used by the getstartaddresses()
	// function to build the address mapping table, *startaddr[].
	// I use the SNES Memory Mapping By: ]SiMKiN[ v3.5
	// but I'm not very sure about this memory map...
	byte b1, b2;
	word a1, a2;
	byte * start;
	boolean continuous;
	word spacing;
} loromarea [] = {
	{ 0x00, 0x3F, 0x0000, 0x1FFF, ram,  	  0,	0 },
	{ 0x00, 0x3F, 0x2000, 0x5FFF, registers,  0,	0 },
	{ 0x00, 0x3F, 0x6000, 0x7FFF, AA_NOTHING, 0,	0 },
	{ 0x00, 0x7D, 0x8000, 0xFFFF, rom,     true,	0 },
	{ 0x40, 0x6F, 0x0000, 0x7FFF, rom,     true,	0 },
	{ 0x70, 0x7D, 0x0000, 0x7FFF, sram, 	  0,	0 },
	{ 0x7E, 0x7F, 0x0000, 0xFFFF, ram,     true,	0 },

	{ 0x80, 0xBF, 0x0000, 0x1FFF, ram, 	  0,	0 },
	{ 0x80, 0xBF, 0x2000, 0x5FFF, registers,  0,	0 },
	{ 0x80, 0xBF, 0x6000, 0x7FFF, AA_NOTHING, 0,	0 },
	{ 0x80, 0xFF, 0x8000, 0xFFFF, rom,    true ,	0 },
	{ 0xC0, 0xEF, 0x0000, 0x7FFF, rom,    true ,	0 },
	{ 0xF0, 0xFE, 0x0000, 0x7FFF, sram,	  0,	0 },
	{ 0 }//tell to getstartaddresses the end of array
}, hiromarea [] = {
	{ 0x00, 0x3F, 0x0000, 0x1FFF, ram,	  0,	0 },
	{ 0x00, 0x3F, 0x2000, 0x5FFF, registers,  0,	0 },
	{ 0x00, 0x2F, 0x6000, 0x7FFF, AA_NOTHING, 0,	0 },
	{ 0x30, 0x3F, 0x6000, 0x7FFF, sram ,	  0,	0 },
	{ 0x00, 0x3F, 0x8000, 0xFFFF, rom+0x8000,  true, 0x8000 },
	{ 0x40, 0x7D, 0x0000, 0xFFFF, rom,     true,	0 },
	{ 0x7E, 0x7F, 0x0000, 0xFFFF, ram,     true,	0 },

	{ 0x80, 0xBF, 0x0000, 0x1FFF, ram, 	  0,	0 },
	{ 0x80, 0xBF, 0x2000, 0x5FFF, registers,  0,	0 },
	{ 0x80, 0xAF, 0x6000, 0x7FFF, AA_NOTHING, 0,	0 },
	{ 0xB0, 0xBF, 0x6000, 0x7FFF, sram,	  0,	0 },
	{ 0x80, 0xBF, 0x8000, 0xFFFF, rom+0x8000,  true, 0x8000 },
	{ 0xC0, 0xFF, 0x0000, 0xFFFF, rom,     true,	0 },
	{ 0 }//tell to getstartaddresses the end of array
};

/*Old version of the memory map*/
/*
#define AA_ROM 	rom
#define AA_HIROM 	(rom + 0x8000)

struct addressarea {
	// this is a representation of the SNES memory map. It's used by the getstartaddresses()
	// function to build the address mapping table, *startaddr[].
	byte b1, b2;
	word a1, a2;
	byte *start;
	boolean continuous;
	word spacing;
} loromarea [] = {
	{ 0x00, 0x3F, 0x0000, 0x1FFF, ram,             },
	{ 0x00, 0x3F, 0x2000, 0x5FFF, registers,       },
	{ 0x00, 0x3F, 0x6000, 0x7FFF, expram,          },
	{ 0x00, 0x6F, 0x8000, 0xFFFF, AA_ROM,    true  },
	{ 0x40, 0x6F, 0x0000, 0x7FFF, AA_NOTHING       },
	{ 0x70, 0x77, 0x0000, 0xFFFF, sram             },
	{ 0x78, 0x7D, 0x0000, 0xFFFF, AA_NOTHING       },
	{ 0x7E, 0x7F, 0x0000, 0xFFFF, ram,       true  },

	{ 0x80, 0xBF, 0x0000, 0x1FFF, ram,             },
	{ 0x80, 0xBF, 0x2000, 0x5FFF, registers,       },
	{ 0x80, 0xBF, 0x6000, 0x7FFF, expram,          },
	{ 0x80, 0xEF, 0x8000, 0xFFFF, AA_ROM,    true  },
	{ 0xC0, 0xEF, 0x0000, 0x7FFF, AA_NOTHING       },
	{ 0xF0, 0xF7, 0x0000, 0xFFFF, sram             },
	{ 0xF8, 0xFD, 0x0000, 0xFFFF, AA_NOTHING       },
	{ 0xFE, 0xFF, 0x0000, 0xFFFF, ram,       true  },
	{ 0 }//tell to getstartaddresses the end of array
}, hiromarea [] = {
	{ 0x00, 0x3F, 0x0000, 0x1FFF, ram,             },
	{ 0x00, 0x3F, 0x2000, 0x5FFF, registers,       },
	{ 0x00, 0x2F, 0x6000, 0x7FFF, expram,          },
	{ 0x30, 0x3F, 0x6000, 0x7FFF, sram,      true  },
	{ 0x00, 0x3F, 0x8000, 0xFFFF, AA_HIROM,  true, 0x8000 },
	{ 0x40, 0x6F, 0x0000, 0xFFFF, AA_ROM,    true  },
	{ 0x70, 0x77, 0x0000, 0xFFFF, sram             },
	{ 0x78, 0x7D, 0x0000, 0xFFFF, AA_NOTHING       },
	{ 0x7E, 0x7F, 0x0000, 0xFFFF, ram,       true  },

	{ 0x80, 0xBF, 0x0000, 0x1FFF, ram,             },
	{ 0x80, 0xBF, 0x2000, 0x5FFF, registers,       },
	{ 0x80, 0xAF, 0x6000, 0x7FFF, expram,          },
	{ 0xB0, 0xBF, 0x6000, 0x7FFF, sram,      true  },
	{ 0x80, 0xBF, 0x8000, 0xFFFF, AA_HIROM,  true, 0x8000 },
	{ 0xC0, 0xFF, 0x0000, 0xFFFF, AA_ROM,    true  },
	{ 0 }//tell to getstartaddresses the end of array
};
*/


/*
 This function use stuct addressarea defined above to build an addresses
 table (startaddr[]).
 This table is use to map an aa.bbbb address to the correct area. With "correct
 area" I mean the location that an address refer according with the snes memory
 map, such as ram, rom, expanded ram, video ram, memory port ecc., infact all
 this
 harware is accessible via memory address
*/
void set_mapping_table_addresses  (boolean hirom)
{
	int x, i;
	unsigned int b, a; // bank, address
	struct addressarea *aa;

	if (hirom) aa = hiromarea;
	else aa = loromarea;

	/*
	with SNESMEM the memory is divided in bank of 0x2000 byte,
		    11 bit(ram,rom,vram, ecc.)   +    13 bit (offset)
	SNESMEM(addr) (startaddr [(addr) >> 13] + (addr) )
	*/
	for (i = 0; i <= 0x7FF; i++) {
			// Get bank and address
		b = (i >> 3);
		a = (i & 7) << 13;
			// Find which of the address areas that bb.aaaa is part of
		for (x = 0; aa[x].start != NULL; x++) {
			if (b >= aa[x].b1 && b <= aa[x].b2 && a >= aa[x].a1 && a <= aa[x].a2)
			{
				break;
			}
		}
		/*
		 when the program use SNESMEM() the most significant 11 bits of the snes address
		 are added to the pointer returned by startaddr[].
		 So now we subtract this value to obtain the correct value for the pointer when
		 we call SNESMEM
		*/
		startaddr [i] = aa[x].start  - (b<<16) - aa[x].a1;

		if (aa[x].continuous)
			startaddr [i] += (b - aa[x].b1) * (aa[x].a2 - aa[x].a1 + 1 + aa[x].spacing);
		

                       	}
}



void showMemoryMap(void)
{

#define ifbetween(mem,size,name)   if( (mapAddr >= mem) && (mapAddr< mem+size) ) memname = name
	dword addr;
	byte* mapAddr;
	char* memname = NULL;
	char* cmps = "null";
	for( addr = 0; addr <= 0xFFFFFF; addr ++)
	{
 		mapAddr = SNESMEM ( addr );
		ifbetween(ram,0x20000,"RAM");
		else ifbetween(rom,0x600000,"ROM");
		else ifbetween(sram,0x8000,"SRAM");
		else ifbetween(registers,0x4000,"REGISTER");
		else ifbetween(expram,0x10000,"unmapped");		
		else memname = "!!!wrong mapping!!!";

		if( strcmp( cmps, memname) )
		{
			printf("%lX mapped in %s", addr-1, cmps);

			printf("\nAddresses: %lX -> ", addr);
			cmps = memname;
		}
	}
	printf(" %lX mapped in %s", addr-1, cmps);

	fflush(stdout);

#undef ifbetween

}

/*call this when the snes need to update joypads registers*/
void updatejoypads(void)
{
      PPU.Joypad1state = getjoypad(0);
      if(PPU.Joypad1state)
      	PPU.Joypad1state |= 0xFFFF0000;
      *REG4218 = (byte) PPU.Joypad1state;
      *REG4219 = (byte) (PPU.Joypad1state>>8);         
      
      PPU.Joypad2state = getjoypad(1);
      if(PPU.Joypad2state)
      	PPU.Joypad2state |= 0xFFFF0000;
      *REG421A = (byte) PPU.Joypad2state;
      *REG421B = (byte) (PPU.Joypad2state>>8);

}


/******************************************************************************
************************************MEMORY MAPPED I/O**************************
******************************************************************************/

word SignExtend [2] = {
    0x00, 0xff00
};

byte* trapregread (byte* ptr, boolean wordread)
{

#ifdef DEBUGGER
 if(  debugger.traceRegs )
 {			
 	printf("\nRead register %x ",ptr-registers+0x2000);
 }
#endif

	switch (ptr-registers+0x2000) {

	      	/*****************************
		****GRAPHICS REGISTERS*****                   --
	              *****************************/
	case 0x2100:
  	case 0x2101:
  	case 0x2102:
   	case 0x2103:
	case 0x2104:
	case 0x2105:
	case 0x2106:
	case 0x2107:
	case 0x2108:
	case 0x2109:
	case 0x210a:
	case 0x210b:
	case 0x210c:
	case 0x210d:
	case 0x210e:
	case 0x210f:
	case 0x2110:
	case 0x2111:
	case 0x2112:
	case 0x2113:
	case 0x2114:
	case 0x2115:
	case 0x2116:
	case 0x2117:
	case 0x2118:
	case 0x2119:
	case 0x211a:
	case 0x211b:
	case 0x211c:
	case 0x211d:
	case 0x211e:
	case 0x211f:
	case 0x2120:
   	case 0x2121:
	case 0x2122:
	case 0x2123:
	case 0x2124:
	case 0x2125:
	case 0x2126:
	case 0x2127:
	case 0x2128:
	case 0x2129:
	case 0x212a:
	case 0x212b:
	case 0x212c:
	case 0x212d:
	case 0x212e:
	case 0x212f:
	case 0x2130:
	case 0x2131:
	case 0x2132:
	case 0x2133:
			DEBUG("\nRead the write only register %x",ptr-registers+0x2000);
			break;
	case 0x2134:
	case 0x2135:
	case 0x2136:
		 // 16bit x 8bit multiply read result.
		if (PPU.Need16x8Mulitply)
		{
			sdword r = (sdword) PPU.MatrixA * (sdword) (PPU.MatrixB >> 8);

			*REG2134 = (byte) r;
			*REG2135 = (byte)(r >> 8);
			*REG2136 = (byte)(r >> 16);
			
			PPU.Need16x8Mulitply = FALSE;
		}
                            break;

	case 0x2137:
		PPU.VBeamPosLatched = SCHEDULER.screen_scanline;
		PPU.VBeamPosLatched |= (PPU.VBeamPosLatched&0xfe)<<8;
	     	PPU.HBeamPosLatched = (word) (((SCHEDULER.cycles_per_scanline - SCHEDULER.cycles)
	     				    * SNES_HCOUNTER_MAX) / SCHEDULER.cycles_per_scanline);
	     	PPU.HBeamPosLatched |= (PPU.HBeamPosLatched&0xfe)<<8;
	     	//PPU.VBeamFlip = 0;
	     	(*REG213F) |= 0x40;
	     	break;
               case 0x2138:
		// Read OAM (sprite) control data
		if(PPU.OAMAddr&0x100){
			if (!(PPU.OAMFlip&1))
			{
				*ptr = PPU.OAMData [(PPU.OAMAddr&0x10f) << 1];
			}
			else
			{
				*ptr = PPU.OAMData [((PPU.OAMAddr&0x10f) << 1) + 1];
				PPU.OAMAddr=(PPU.OAMAddr+1)&0x1ff;
			}
		} else {
			if (!(PPU.OAMFlip&1))
			{
				*ptr = PPU.OAMData [PPU.OAMAddr << 1];
			}
			else
			{
				*ptr = PPU.OAMData [(PPU.OAMAddr << 1) + 1];
				++PPU.OAMAddr;
			}
		}
	    	PPU.OAMFlip ^= 1;
		break;
                             
            	case 0x2139:
		// Read vram low byte
		if ( PPU.FirstVRAMRead)
			*ptr = PPU.VRAM[(PPU.VMA.Address << 1)&0xFFFF];
		else
			if (PPU.VMA.FullGraphicCount)
			{
				dword addr = PPU.VMA.Address - 1;
				dword rem = addr & PPU.VMA.Mask1;
				dword address = (addr & ~PPU.VMA.Mask1) +
					(rem >> PPU.VMA.Shift) +
					((rem & (PPU.VMA.FullGraphicCount - 1)) << 3);
				*ptr = PPU.VRAM [((address << 1) - 2) & 0xFFFF];
			}
			else
				*ptr = PPU.VRAM[((PPU.VMA.Address << 1) - 2) & 0xffff];

		if ( ! PPU.VMA.High)
		{
			PPU.VMA.Address += PPU.VMA.Increment;
			PPU.FirstVRAMRead = FALSE;
		}
		break;

	case 0x213A:
                            // Read vram high byte
		if ( PPU.FirstVRAMRead)
			*ptr = PPU.VRAM[((PPU.VMA.Address << 1) + 1) & 0xffff];
		else
			if (PPU.VMA.FullGraphicCount)
			{
				dword addr = PPU.VMA.Address - 1;
				dword rem = addr & PPU.VMA.Mask1;
				dword address = (addr & ~PPU.VMA.Mask1) +
					(rem >> PPU.VMA.Shift) +
					((rem & (PPU.VMA.FullGraphicCount - 1)) << 3);
				*ptr = PPU.VRAM [((address << 1) - 1) & 0xFFFF];
			}
			else
				*ptr = PPU.VRAM[((PPU.VMA.Address << 1) - 1) & 0xFFFF];
				
		if ( PPU.VMA.High )
		{
			PPU.VMA.Address += PPU.VMA.Increment;
			PPU.FirstVRAMRead = FALSE;
		}
		break;

	case 0x213B:
	    	// Read palette data
	    	if (PPU.CGFLIPRead)
			*ptr = PPU.CGDATA [PPU.CGADD++] >> 8;
	    	else
			*ptr = PPU.CGDATA [PPU.CGADD] & 0xff;

	    	PPU.CGFLIPRead ^= 1;
	    	break;

	case 0x213C:
	    	// Horizontal counter value 0-339
	    	if (PPU.HBeamFlip)
			*ptr = PPU.HBeamPosLatched >> 8;
	    	else
			*ptr = (byte)PPU.HBeamPosLatched;
	    	PPU.HBeamFlip ^= 1;
	    	break;

	case 0x213D:
	    	// Vertical counter value 0-262
	    	if (PPU.VBeamFlip)
			*ptr = PPU.VBeamPosLatched >> 8;
	    	else
			*ptr = (byte)PPU.VBeamPosLatched;
	    	PPU.VBeamFlip ^= 1;
	    	break;
              case 0x213E:
              	// PPU time and range over flags
	    	FLUSH_REDRAW ();
	    	*ptr = MAX_5C77_VERSION ;
		break;
              case 0x213F:
	    	// NTSC/PAL and which field flags
	    	PPU.VBeamFlip = PPU.HBeamFlip = 0;
		//neviksti found a 2 and a 3 here. SNEeSe uses a 3.
	    	*ptr = ((snes_settings.is_ntsc ? 0 : 0x10 ) | (*ptr & 0xc0)| MAX_5C78_VERSION);
	    	break;

              case 0x2140: case 0x2141: case 0x2142: case 0x2143:
	case 0x2144: case 0x2145: case 0x2146: case 0x2147:
	case 0x2148: case 0x2149: case 0x214a: case 0x214b:
	case 0x214c: case 0x214d: case 0x214e: case 0x214f:
	case 0x2150: case 0x2151: case 0x2152: case 0x2153:
	case 0x2154: case 0x2155: case 0x2156: case 0x2157:
	case 0x2158: case 0x2159: case 0x215a: case 0x215b:
	case 0x215c: case 0x215d: case 0x215e: case 0x215f:
	case 0x2160: case 0x2161: case 0x2162: case 0x2163:
	case 0x2164: case 0x2165: case 0x2166: case 0x2167:
	case 0x2168: case 0x2169: case 0x216a: case 0x216b:
	case 0x216c: case 0x216d: case 0x216e: case 0x216f:
	case 0x2170: case 0x2171: case 0x2172: case 0x2173:
	case 0x2174: case 0x2175: case 0x2176: case 0x2177:
	case 0x2178: case 0x2179: case 0x217a: case 0x217b:
	case 0x217c: case 0x217d: case 0x217e: case 0x217f:
              	//APU read
              	
		    if (((ptr-registers+0x2000) & 3) < 2)
		    {
			int r = rand ();
			if (r & 2)
			{
			    if (r & 4)
				*ptr= (((ptr-registers+0x2000)&3) == 1 ? 0xaa : 0xbb);
			    else
				*ptr =((r >> 3) & 0xff);
			}
		    }
		    else
		    {
			int r = rand ();
			if (r & 2)
			    *ptr = ((r >> 3) & 0xff);
		    }
#ifdef SOUND_SKIP
		{
		//static dword saved_pc =0;
		
		CPU.branch_skip = TRUE;
		CPU.skip_method = (++CPU.skip_method %4);
                           
                            /*
                            if( saved_pc == getPC() )
                            {
                            	CPU.skip_method = (++CPU.skip_method % 3);
                            }
                            else
                            {
                             	saved_pc = getPC();
                             	CPU.skip_method = 0;
                           }*/
                          }
#endif		
                    
#ifdef DEBUGGER
		 if(  debugger.traceAudio )
		 {			
		 	printf("\nRead AUDIO register %x at code address: 0x%06lX",ptr-registers+0x2000,getPC() );
		 }
#endif
		break;

               case 0x2180:
		   // Read WRAM
		*ptr = ram [PPU.WRAM];
	    	PPU.WRAM++;
        		PPU.WRAM &= 0x1FFFF;
		break;

	case 0x2181:
	case 0x2182:
	case 0x2183:
		DEBUG("\nRead the write only register %x",ptr-registers+0x2000);
		break;

		/***********************************
		****IRQ, NMI, HDMA, DMA etc. etc.*****
		***********************************/
	case 0x4016:
		//old style joypad 1 reading
		*ptr = (( PPU.Joypad1state >> ( PPU.Joypad1ButtonReadPos++ ^ 15)) & 1);
		break;
		    
	case 0x4017:
		//old style joypad 2 reading
		*ptr = (( PPU.Joypad2state >> ( PPU.Joypad2ButtonReadPos++ ^ 15)) & 1);
		break;
	
	case 0x4200:
	case 0x4201:
	case 0x4202:
	case 0x4203:
	case 0x4204:
	case 0x4205:
	case 0x4206:
	case 0x4207:
	case 0x4208:
	case 0x4209:
	case 0x420a:
	case 0x420b:
	case 0x420c:
	case 0x420d:
	case 0x420e:
	case 0x420f:
		 DEBUG("\nRead the write only register %x",ptr-registers+0x2000);
		break;

	case 0x4210:
		*ptr = ((PPU.reg4210&0x80)|MAX_5A22_VERSION);
		PPU.reg4210 = MAX_5A22_VERSION;
		break;

	case 0x4211:
		// IRQ ocurred flag (reset on read or write)
		*ptr = CPU.irq ? 0x80 : 0;
		CPU.irq = FALSE;
		break;
	case 0x4212:
		{
		int HBlankStart;

		/*joypad ready ?*/
                            if ( SCHEDULER.screen_scanline >= PPU.screen_height + FIRST_VISIBLE_LINE  &&
			SCHEDULER.screen_scanline < PPU.screen_height + FIRST_VISIBLE_LINE + 3)
			*ptr = 1;
		else 	*ptr = 0;

		HBlankStart = ( SCHEDULER.cycles_per_scanline == SCHEDULER.slowrom_cycles_per_scanline ) ?
					SCHEDULER.slowromHBlankStart  :  SCHEDULER.fastromHBlankStart ;
		/*H-blank state?*/
    		(*ptr) |= SCHEDULER.cycles <= HBlankStart ? 0x40 : 0;

		/*V-blank state? */
   	 	if ( SCHEDULER.screen_scanline >=  PPU.screen_height + FIRST_VISIBLE_LINE )
			(*ptr) |= 0x80; /* XXX: 0x80 or 0xc0 ? */

		}
		break;

	case 0x4213:
		// I/O port input - returns 0 wherever $4201 is 0, and 1 elsewhere
		// unless something else pulls it down (i.e. a gun)
		DEBUG("\nRead unimplemented register %x",ptr-registers+0x2000);
		break;

	case 0x4214:
	case 0x4215:
		// Quotient of divide result
		break;
	case 0x4216:
	case 0x4217:
		// Multiplcation result (for multiply) or remainder of
		// divison.
		break;

	case 0x4218:
	case 0x4219:
	case 0x421a:
	case 0x421b:
		// Joypads 1-2 button and direction state.
		break;
		
	case 0x421c:
	case 0x421d:
	case 0x421e:
	case 0x421f:
		// Joypads 3-4 button and direction state.
		DEBUG("\nRead unimplemented joypad register %x",ptr-registers+0x2000);
		break;
                  
	case 0x4300:
	case 0x4310:
	case 0x4320:
	case 0x4330:
	case 0x4340:
	case 0x4350:
	case 0x4360:
	case 0x4370:
  		DEBUG("\n H-DMA letto registro 43x0");
		break;

	case 0x4301:
	case 0x4311:
	case 0x4321:
	case 0x4331:
	case 0x4341:
	case 0x4351:
	case 0x4361:
	case 0x4371:
	  		DEBUG("\n H-DMA letto registro 43x1");
			break;
	case 0x4302:
	case 0x4312:
	case 0x4322:
	case 0x4332:
	case 0x4342:
	case 0x4352:
	case 0x4362:
	case 0x4372:
	  		DEBUG("\n H-DMA letto registro 43x2");
			break;
	case 0x4303:
	case 0x4313:
	case 0x4323:
	case 0x4333:
	case 0x4343:
	case 0x4353:
	case 0x4363:
	case 0x4373:
	  		DEBUG("\n H-DMA letto registro 43x3");
			break;
	case 0x4304:
	case 0x4314:
	case 0x4324:
	case 0x4334:
	case 0x4344:
	case 0x4354:
	case 0x4364:
	case 0x4374:
	 		DEBUG("\n H-DMA letto registro 43x4");
			break;
	case 0x4305:
	case 0x4315:
	case 0x4325:
	case 0x4335:
	case 0x4345:
	case 0x4355:
	case 0x4365:
	case 0x4375:
			DEBUG("\n H-DMA letto registro 43x5");
			break;
	case 0x4306:
	case 0x4316:
	case 0x4326:
	case 0x4336:
	case 0x4346:
	case 0x4356:
	case 0x4366:
	case 0x4376:
			DEBUG("\n H-DMA letto registro 43x6");
			break;
	case 0x4307:
	case 0x4317:
	case 0x4327:
	case 0x4337:
	case 0x4347:
	case 0x4357:
	case 0x4367:
	case 0x4377:
	 		DEBUG("\n H-DMA letto registro 43x7");
			break;
	case 0x4308:
	case 0x4318:
	case 0x4328:
	case 0x4338:
	case 0x4348:
	case 0x4358:
	case 0x4368:
	case 0x4378:
			DEBUG("\n H-DMA letto registro 43x8");
			break;
	case 0x4309:
	case 0x4319:
	case 0x4329:
	case 0x4339:
	case 0x4349:
	case 0x4359:
	case 0x4369:
	case 0x4379:
	 		DEBUG("\n H-DMA letto registro 43x9");
			break;
	case 0x430A:
	case 0x431A:
	case 0x432A:
	case 0x433A:
	case 0x434A:
	case 0x435A:
	case 0x436A:
	case 0x437A:
	  		DEBUG("\n H-DMA letto registro 43xA");
			break;
	}

	if (wordread)
		trapregread (ptr + 1, false);

	return ptr;//return the pointer to that memory
}


byte* trapregwrite (byte *ptr, boolean wordwrite)
{

#ifdef DEBUGGER
 if(  debugger.traceRegs )
 {			
 	printf("\nWrote register %x",ptr-registers+0x2000);
 }
#endif

	switch (ptr-registers+0x2000) {

	      	/*****************************
		****GRAPHICS REGISTERS*****
	              *****************************/
	case 0x2100:
		// Brightness and screen blank bit
		if( *ptr != PPU.reg2100)
		{
			FLUSH_REDRAW ();
			if (PPU.Brightness != ( *ptr & 0xF))
			{
				PPU.ColorsChanged = TRUE;
				//PPU.DirectColourMapsNeedRebuild = TRUE;
				PPU.Brightness = *ptr & 0xF;
			}
			
			if ( (PPU.reg2100 & 0x80) != ( *ptr & 0x80))
			{
				PPU.ForcedBlanking = (*ptr >> 7) & 1;
			}                               
			                      
			PPU.reg2100 = *ptr;
		}
		break;

	case 0x2101:
		// Sprite (OBJ) tile address
		if ( *ptr != PPU.reg2101 )
		{
			FLUSH_REDRAW ();
			PPU.OBJNameBase   = (*ptr & 3) << 14;
			PPU.OBJNameSelect = ((*ptr >> 3) & 3) << 13;
			PPU.OBJSizeSelect = (*ptr >> 5) & 7;
			PPU.OBJChanged = TRUE;
			PPU.reg2101 = *ptr;
#ifdef DEBUGGER
			if(PPU.OBJNameSelect)
				DEBUG("\n Sprites have Name select != 0");
#endif
		}
		break;

	case 0x2102:
		// Sprite write address (low)
		PPU.OAMAddr = (PPU.OAMAddr & 0x0100 ) | *ptr;
		PPU.OAMFlip = 0;
		PPU.SavedOAMAddr = PPU.OAMAddr;
		if (PPU.OAMPriorityRotation && PPU.FirstSprite != (PPU.OAMAddr >> 1))
		{
			PPU.FirstSprite = (PPU.OAMAddr&0xFE) >> 1;
			PPU.OBJChanged = TRUE;
		}
		break;

	case 0x2103:
		// Sprite register write address (high), sprite priority rotation
		// bit.
		PPU.OAMAddr &= 0x00FF;
		PPU.OAMAddr |= (*ptr & 1) << 8;

		PPU.OAMPriorityRotation=(*ptr & 0x80)? 1 : 0;

		if (PPU.OAMPriorityRotation && PPU.FirstSprite != (PPU.OAMAddr >> 1))
		{
			PPU.FirstSprite = (PPU.OAMAddr&0xFE) >> 1;
			PPU.OBJChanged = TRUE;
		}
		PPU.OAMFlip = 0;
		PPU.SavedOAMAddr = PPU.OAMAddr;
		break;

	case 0x2104:
		// Sprite register write
		if (PPU.OAMAddr & 0x100)//i.e. accessing the 32byte obj attributes table
		{
			int addr = ((PPU.OAMAddr & 0x10f) << 1) + (PPU.OAMFlip & 1);
			if ( *ptr != PPU.OAMData [addr])
			{
				struct SOBJ *pObj;
				FLUSH_REDRAW ();
				PPU.OAMData [addr] = *ptr;
				PPU.OBJChanged = TRUE;

				// X position high bit, and sprite size (x4)
				pObj = &PPU.OBJ [ ((addr & 0x1f) << 2) ];

				pObj->HPos = (pObj->HPos & 0xFF) | SignExtend[(*ptr >> 0) & 1];
				pObj++->Size = *ptr & 2;
				pObj->HPos = (pObj->HPos & 0xFF) | SignExtend[(*ptr >> 2) & 1];
				pObj++->Size = *ptr & 8;
				pObj->HPos = (pObj->HPos & 0xFF) | SignExtend[(*ptr >> 4) & 1];
				pObj++->Size = *ptr & 32;
				pObj->HPos = (pObj->HPos & 0xFF) | SignExtend[(*ptr >> 6) & 1];
				pObj->Size = *ptr & 128;
			}
			PPU.OAMFlip ^= 1;
			if(!(PPU.OAMFlip & 1))
			{
				++PPU.OAMAddr;
				PPU.OAMAddr &= 0x1ff;
			}
		}
		else if(!(PPU.OAMFlip & 1))
		{
			PPU.OAMWriteRegister &= 0xff00;
			PPU.OAMWriteRegister |= *ptr;
			PPU.OAMFlip |= 1;
		}
		else//i.e. accessing 512 byte obj attributes table
		{
			int addr;
			byte lowbyte, highbyte;
			PPU.OAMWriteRegister &= 0x00ff;
			lowbyte = (byte) (PPU.OAMWriteRegister);
			highbyte = *ptr;
			PPU.OAMWriteRegister |= *ptr << 8;

			addr = (PPU.OAMAddr << 1);

			if (lowbyte != PPU.OAMData [addr] ||
				highbyte != PPU.OAMData [addr+1])
			{
				FLUSH_REDRAW ();
				PPU.OAMData [addr] = lowbyte;
				PPU.OAMData [addr+1] = highbyte;
				PPU.OBJChanged = TRUE;
				if (addr & 2)
				{
					// Tile
					PPU.OBJ[addr = PPU.OAMAddr >> 1].Name = PPU.OAMWriteRegister & 0x1ff;

					// priority, h and v flip.
					PPU.OBJ[addr].Palette = (highbyte >> 1) & 7;
					PPU.OBJ[addr].Priority = (highbyte >> 4) & 3;
					PPU.OBJ[addr].HFlip = (highbyte >> 6) & 1;
					PPU.OBJ[addr].VFlip = (highbyte >> 7) & 1;
					}
				else
				{
					// X position (low)
					PPU.OBJ[addr = PPU.OAMAddr >> 1].HPos &= 0xFF00;
					PPU.OBJ[addr].HPos |= lowbyte;

					// Sprite Y position
					PPU.OBJ[addr].VPos = highbyte;
				}
			}
			PPU.OAMFlip &= ~1;
			++PPU.OAMAddr;
		}
		break;

	case 0x2105:
		// Screen mode (0 - 7), background tile sizes and background 3
		// priority
		if ( *ptr != PPU.reg2105 )
		{
			FLUSH_REDRAW ();
			PPU.BG[0].BGSize = (*ptr >> 4) & 1;
			PPU.BG[1].BGSize = (*ptr >> 5) & 1;
			PPU.BG[2].BGSize = (*ptr >> 6) & 1;
			PPU.BG[3].BGSize = (*ptr >> 7) & 1;
			PPU.BGMode = *ptr & 7;
			// BJ: BG3Priority only takes effect if BGMode==1 and the bit is set
			PPU.BG3Priority  = ((*ptr & 0x0f) == 0x09);
			PPU.reg2105 = *ptr;
		}
		break;
			
	case 0x2106:
		// Mosaic pixel size and enable
		if ( *ptr != PPU.reg2106 )
		{
			FLUSH_REDRAW ();
			PPU.Mosaic = (*ptr >> 4) + 1;
			PPU.BGMosaic [0] = (*ptr & 1) && PPU.Mosaic > 1;
			PPU.BGMosaic [1] = (*ptr & 2) && PPU.Mosaic > 1;
			PPU.BGMosaic [2] = (*ptr & 4) && PPU.Mosaic > 1;
			PPU.BGMosaic [3] = (*ptr & 8) && PPU.Mosaic > 1;
			
			PPU.reg2106 = *ptr;
			DEBUG("\nWrote Mosaic BG unimplemented register %x",ptr-registers+0x2000);
		}
		break;

	case 0x2107:
		// [BG0SC]
		if ( *ptr != PPU.reg2107 )
		{
			FLUSH_REDRAW ();
			PPU.BG[0].SCSize = *ptr & 3;
			PPU.BG[0].SCBase = ( *ptr & 0x7c) << 8;
			PPU.reg2107 = *ptr;
		}
		break;

	case 0x2108:
		// [BG1SC]
		if ( *ptr != PPU.reg2108 )
		{
			FLUSH_REDRAW ();
			PPU.BG[1].SCSize = *ptr & 3;
			PPU.BG[1].SCBase = ( *ptr & 0x7c) << 8;
			PPU.reg2108 = *ptr;
		}
		break;

 	case 0x2109:
		// [BG2SC]
		if ( *ptr != PPU.reg2109 )
		{
			FLUSH_REDRAW ();
			PPU.BG[2].SCSize = *ptr & 3;
			PPU.BG[2].SCBase = ( *ptr & 0x7c) << 8;
			PPU.reg2109 = *ptr;
		}
		break;

	case 0x210A:
		// [BG3SC]
		if ( *ptr != PPU.reg210A )
		{
			FLUSH_REDRAW ();
			PPU.BG[3].SCSize = *ptr & 3;
			PPU.BG[3].SCBase = ( *ptr & 0x7c) << 8;
			PPU.reg210A = *ptr;
		}
		break;

              case 0x210B:
		// [BG01NBA]
		if ( *ptr != PPU.reg210B )
		{
				FLUSH_REDRAW ();
				PPU.BG[0].NameBase    = (*ptr & 7) << 12;
				PPU.BG[1].NameBase    = ((*ptr >> 4) & 7) << 12;
				PPU.reg210B = *ptr;
		}
		break;

	case 0x210C:
		// [BG23NBA]
		if ( *ptr != PPU.reg210C )
		{
				FLUSH_REDRAW ();
				PPU.BG[2].NameBase    = (*ptr & 7) << 12;
				PPU.BG[3].NameBase    = ((*ptr >> 4) & 7) << 12;
				PPU.reg210C = *ptr;
		}
		break;

	case 0x210D:
		PPU.BG[0].HOffset = ((PPU.BG[0].HOffset >> 8) & 0xff) |
			((word) (*ptr) << 8);
		break;

	case 0x210E:
		PPU.BG[0].VOffset = ((PPU.BG[0].VOffset >> 8) & 0xff) |
			((word) (*ptr) << 8);
		break;

	case 0x210F:
		PPU.BG[1].HOffset = ((PPU.BG[1].HOffset >> 8) & 0xff) |
			((word) (*ptr) << 8);
		break;

	case 0x2110:
		PPU.BG[1].VOffset = ((PPU.BG[1].VOffset >> 8) & 0xff) |
			((word) (*ptr) << 8);
		break;

	case 0x2111:
		PPU.BG[2].HOffset = ((PPU.BG[2].HOffset >> 8) & 0xff) |
			((word) (*ptr) << 8);
		break;

	case 0x2112:
		PPU.BG[2].VOffset = ((PPU.BG[2].VOffset >> 8) & 0xff) |
			((word) (*ptr) << 8);
		break;

	case 0x2113:
		PPU.BG[3].HOffset = ((PPU.BG[3].HOffset >> 8) & 0xff) |
			((word) (*ptr) << 8);
		break;

	case 0x2114:
		PPU.BG[3].VOffset = ((PPU.BG[3].VOffset >> 8) & 0xff) |
			((word) (*ptr) << 8);
		break;

	case 0x2115:
		// VRAM byte/word access flag and increment
		PPU.VMA.High = (*ptr & 0x80) == 0 ? FALSE : TRUE;
		switch (*ptr & 3)
		{
		  case 0:
			PPU.VMA.Increment = 1;
			break;
		  case 1:
			PPU.VMA.Increment = 32;
			break;
		  case 2:
			DEBUG("\nSelected increment 64 of register 0x2115. We set 128 instead");
			PPU.VMA.Increment = 128;
			break;
		  case 3:
			PPU.VMA.Increment = 128;
			break;
		}

		if ( *ptr & 0x0c)
		{
			static word IncCount [4] = { 0, 32, 64, 128 };
			static word Shift [4] = { 0, 5, 6, 7 };
                                          byte i;

			PPU.VMA.Increment = 1;
			i = (*ptr & 0x0c) >> 2;
			PPU.VMA.FullGraphicCount = IncCount [i];
			PPU.VMA.Mask1 = IncCount [i] * 8 - 1;
			PPU.VMA.Shift = Shift [i];
			DEBUG("\nFull graphic bits of register 0x2115 !=0 ");
		}
		else
			PPU.VMA.FullGraphicCount = 0;
		break;

	case 0x2116:
		// VRAM read/write address (low)
		PPU.VMA.Address &= 0xFF00;
		PPU.VMA.Address |= *ptr;
		PPU.FirstVRAMRead = TRUE;
		break;

	case 0x2117:
		// VRAM read/write address (high)
		PPU.VMA.Address &= 0x00FF;
		PPU.VMA.Address |= (*ptr << 8);
		PPU.FirstVRAMRead = TRUE;
		break;

	case 0x2118:
		// VRAM write data (low)
		PPU.FirstVRAMRead = TRUE;
		{
    		dword address;
    		if (PPU.VMA.FullGraphicCount)
   		{
			dword rem = PPU.VMA.Address & PPU.VMA.Mask1;
			address = (((PPU.VMA.Address & ~PPU.VMA.Mask1) +
					 (rem >> PPU.VMA.Shift) +
					 ((rem & (PPU.VMA.FullGraphicCount - 1)) << 3)) << 1) & 0xffff;
			PPU.VRAM [address] = *ptr;
		}
		else
		{
			PPU.VRAM[address = (PPU.VMA.Address << 1) & 0xFFFF] = *ptr;
		}
		if ( ! PPU.VMA.High)
		{
			PPU.VMA.Address += PPU.VMA.Increment;
		}
		
		TileCache.valid [TILE_2BIT][address >> 4] = 0;
    		TileCache.valid [TILE_4BIT][address >> 5] = 0;
    		TileCache.valid [TILE_8BIT][address >> 6] = 0;

		}
		break;

	case 0x2119:
	              // VRAM write data (high)
		PPU.FirstVRAMRead = TRUE;
		{
	    	dword address;
    		if (PPU.VMA.FullGraphicCount)
		{
			dword rem = PPU.VMA.Address & PPU.VMA.Mask1;
			address = ((((PPU.VMA.Address & ~PPU.VMA.Mask1) +
				    (rem >> PPU.VMA.Shift) +
				    ((rem & (PPU.VMA.FullGraphicCount - 1)) << 3)) << 1) + 1) & 0xFFFF;
			PPU.VRAM [address] = *ptr;        
	    	}
    		else
		{
			PPU.VRAM[address = ((PPU.VMA.Address << 1) + 1) & 0xFFFF] = *ptr;
	   	}

    		if(PPU.VMA.High)
    		{
			PPU.VMA.Address += PPU.VMA.Increment;
    		}
    		
		TileCache.valid [TILE_2BIT][address >> 4] = 0;
    		TileCache.valid [TILE_4BIT][address >> 5] = 0;
    		TileCache.valid [TILE_8BIT][address >> 6] = 0;

		}
                            break;
               
	case 0x211a:
		// Mode 7 outside rotation area display mode and flipping
		if ( *ptr != PPU.reg211A )
		{
			FLUSH_REDRAW ();
			PPU.Mode7Repeat = (*ptr) >> 6;
			if (PPU.Mode7Repeat == 1)
					PPU.Mode7Repeat = 0;
			PPU.Mode7VFlip = ( (*ptr) & 2) >> 1;
			PPU.Mode7HFlip = (*ptr) & 1;
			
			PPU.reg211A = *ptr;
		}
		break;
		
		break;
	case 0x211b:
		// Mode 7 matrix A (low & high)
		PPU.MatrixA = ((PPU.MatrixA >> 8) & 0xff) | (*ptr << 8);
		PPU.Need16x8Mulitply = TRUE;
		break;
	  case 0x211c:
		// Mode 7 matrix B (low & high)
		PPU.MatrixB = ((PPU.MatrixB >> 8) & 0xff) | (*ptr << 8);
		PPU.Need16x8Mulitply = TRUE;
		break;
	  case 0x211d:
		// Mode 7 matrix C (low & high)
		PPU.MatrixC = ((PPU.MatrixC >> 8) & 0xff) | (*ptr << 8);
		break;
	  case 0x211e:
		// Mode 7 matrix D (low & high)
		PPU.MatrixD = ((PPU.MatrixD >> 8) & 0xff) | (*ptr << 8);
		break;
	  case 0x211f:
		// Mode 7 centre of rotation X (low & high)
		PPU.CentreX = ((PPU.CentreX >> 8) & 0xff) | (*ptr << 8);
		break;
	  case 0x2120:
		// Mode 7 centre of rotation Y (low & high)
		PPU.CentreY = ((PPU.CentreY >> 8) & 0xff) | (*ptr << 8);
		break;                            

	case 0x2121:
		// CG-RAM address
		PPU.CGFLIP = 0;
		PPU.CGFLIPRead = 0;
		PPU.CGADD = *ptr;
		break;
	case 0x2122:
	              // CG-RAM (palette) write
 		if (PPU.CGFLIP)
		 {
				if ( (*ptr & 0x7f) != (PPU.CGDATA[PPU.CGADD] >> 8) )
				{
				  	FLUSH_REDRAW ();
			    		PPU.CGDATA[PPU.CGADD] &= 0x00FF;
			    		PPU.CGDATA[PPU.CGADD] |= ( *ptr & 0x7f ) << 8;
			    		//UpdateDisplayPaletteColor(PPU.CGDATA[PPU.CGADD] ,PPU.CGADD );
			    		PPU.ColorsChanged = TRUE;
				}
				PPU.CGADD++;
		   }
		   else
		   {
				if ( *ptr != (byte) (PPU.CGDATA[PPU.CGADD] & 0xff))
				{
					FLUSH_REDRAW ();
			  	  	PPU.CGDATA[PPU.CGADD] &= 0x7F00;
			    		PPU.CGDATA[PPU.CGADD] |= *ptr;
			    		//UpdateDisplayPaletteColor(PPU.CGDATA[PPU.CGADD] ,PPU.CGADD );
			    		PPU.ColorsChanged = TRUE;
				}
		   }
    		PPU.CGFLIP ^= 1;
		break;

	case 0x2123:
			// Window 1 and 2 enable for backgrounds 1 and 2
	case 0x2124:
			// Window 1 and 2 enable for backgrounds 3 and 4
	case 0x2125:
			// Window 1 and 2 enable for objects and colour window
	case 0x2126:
			// Window 1 left position
	case 0x2127:
			// Window 1 right position
	case 0x2128:
			// Window 2 left position
	case 0x2129:
			// Window 2 right position
	case 0x212a:
			// Windows 1 & 2 overlap logic for backgrounds 1 - 4
	case 0x212b:
			// Windows 1 & 2 overlap logic for objects and colour window
			
	  		DEBUG("\nWrote unimplemented register %x",ptr-registers+0x2000);
			break;
			
	case 0x212c:
			// Main screen designation (backgrounds 1 - 4 and objects)
			PPU.reg212C = *ptr;
			break;
	case 0x212d:
			// Sub-screen designation (backgrounds 1 - 4 and objects)
	case 0x212e:
			// Window mask designation for main screen ?
	case 0x212f:
			// Window mask designation for sub-screen ?
	case 0x2130:
			// Fixed colour addition or screen addition
	case 0x2131:
			// Colour addition or subtraction select
	case 0x2132:
	case 0x2133:
			// Screen settings
		if ( *ptr != PPU.reg2133 )
		{
			FLUSH_REDRAW ();
			if ( *ptr & 0x40)
			{
				PPU.Mode7PriorityMask = 0x80;
				PPU.Mode7Mask = 0x7F;
			}
			else
			{
				PPU.Mode7PriorityMask = 0x00;
				PPU.Mode7Mask = 0xFF;
			}


			if ( *ptr & 0x04)
			{
				PPU.screen_height = SNES_SCREEN_HEIGHT_EXTENDED;
                                          }
			else PPU.screen_height = SNES_SCREEN_HEIGHT;

			PPU.reg2133 = *ptr;
		}
  		DEBUG("\nWrote unimplemented register %x",ptr-registers+0x2000);
		break;

	case 0x2134:
	case 0x2135:
	case 0x2136:
		// Matrix 16bit x 8bit multiply result (read-only)
		PPU.Need16x8Mulitply = TRUE;
  		DEBUG("\n!!!Wrote read only register %x",ptr-registers+0x2000);
		break;
							
	case 0x2137:
			// Software latch for horizontal and vertical timers (read-only)
	case 0x2138:
			// OAM read data (read-only)
	case 0x2139:
	case 0x213a:
			// VRAM read data (read-only)
	case 0x213b:
			// CG-RAM read data (read-only)
	case 0x213c:
	case 0x213d:
			// Horizontal and vertical (low/high) read counter (read-only)
	case 0x213e:
			// PPU status (time over and range over)
	case 0x213f:
			// NTSC/PAL select and field (read-only)

  		DEBUG("\n!!!Wrote read only register %x",ptr-registers+0x2000);
		break;
		
	case 0x2140: case 0x2141: case 0x2142: case 0x2143:
	case 0x2144: case 0x2145: case 0x2146: case 0x2147:
	case 0x2148: case 0x2149: case 0x214a: case 0x214b:
	case 0x214c: case 0x214d: case 0x214e: case 0x214f:
	case 0x2150: case 0x2151: case 0x2152: case 0x2153:
	case 0x2154: case 0x2155: case 0x2156: case 0x2157:
	case 0x2158: case 0x2159: case 0x215a: case 0x215b:
	case 0x215c: case 0x215d: case 0x215e: case 0x215f:
	case 0x2160: case 0x2161: case 0x2162: case 0x2163:
	case 0x2164: case 0x2165: case 0x2166: case 0x2167:
	case 0x2168: case 0x2169: case 0x216a: case 0x216b:
	case 0x216c: case 0x216d: case 0x216e: case 0x216f:
	case 0x2170: case 0x2171: case 0x2172: case 0x2173:
	case 0x2174: case 0x2175: case 0x2176: case 0x2177:
	case 0x2178: case 0x2179: case 0x217a: case 0x217b:
	case 0x217c: case 0x217d: case 0x217e: case 0x217f:
/*
#ifdef DEBUGGER
		 if(  debugger.traceAudio )
		 {			
		 	printf("\nWrote unimplemented AUDIO register %x",ptr-registers+0x2000);
		 }
#endif
*/
  		break;
		
	case 0x2180:
		ram [ PPU.WRAM ] = *ptr;
		PPU.WRAM++; 
        		PPU.WRAM &= 0x1FFFF;
		break;

	case 0x2181:
		PPU.WRAM &= 0x1FF00;
		PPU.WRAM |= *ptr;
		break;
	case 0x2182:
		PPU.WRAM &= 0x100FF;
		PPU.WRAM |= (*ptr << 8);
		break;
	case 0x2183:
		PPU.WRAM &= 0x0FFFF;
		PPU.WRAM |= (*ptr << 16);
		PPU.WRAM &= 0x1FFFF;
		break;
                           
		/***********************************
		****IRQ, NMI, HDMA, DMA etc. etc.*****
		***********************************/
	case 0x4016:
	              {
	                static byte reg4016 = 0; 
		  if ( ( *ptr & 1) && !(reg4016 & 1) )
		  {
			PPU.Joypad1ButtonReadPos = 0;
			PPU.Joypad2ButtonReadPos = 0;
		  }
		  reg4016 = *ptr;
		}
		break;
	case 0x4017:
		break;
		
	case 0x4200:// NMI, V & H IRQ and joypad reading enable flags

		CPU.irq = false;//is it correct?

		if (*ptr & 0x20){
			DEBUG("\nV-IRQ enable");
			PPU.VTimerEnabled = TRUE;

		}else{
			DEBUG("\nV-IRQ disable");
			PPU.VTimerEnabled = FALSE;
		}

		if (*ptr & 0x10){
			DEBUG("\nH-IRQ enable");
			PPU.HTimerEnabled = TRUE;
			setHIRQ ();
		}else{
			DEBUG("\nH-IRQ disable");
			PPU.HTimerEnabled = FALSE;
		}

		if (*ptr & 0x80){
			DEBUG("\nNMI enable");
			PPU.NMIEnable = TRUE;
		}else{
			DEBUG("\nNMI disable");
			PPU.NMIEnable = FALSE;
		}
		break;
		
	case 0x4201: // I/O Port out
		*REG4213 = *ptr;//if you want to run Super metroid comment this line
		break;

	case 0x4202:
		// Multiplier (for multply)
		break;
	case 0x4203:
		{
		// Multiplicand
		dword res = (*REG4202) * (*ptr);

		*REG4216 = (byte) res;
		*REG4217 = (byte) (res >> 8);
		break;
		}

	case 0x4204:
	case 0x4205:
		// Low and high muliplier (for divide)
		break;
	case 0x4206:
		{
		// Divisor
		word a = (*REG4204) + (*REG4205 << 8);
		word div = (*ptr) ? a / (*ptr) : 0xFFFF;
		word rem = (*ptr) ? a % (*ptr) : a;
		
		*REG4214 = (byte)div;
		*REG4215 = div >> 8;
		*REG4216 = (byte)rem;
		*REG4217 = rem >> 8;
		break;
		}

	case 0x4207:
		{
		int d = PPU.IRQHBeamPos;
		PPU.IRQHBeamPos = (PPU.IRQHBeamPos & 0xFF00) | *ptr;

		if (PPU.HTimerEnabled && PPU.IRQHBeamPos != d)
			setHIRQ ();
		}
		break;

	case 0x4208:
		{
		int d = PPU.IRQHBeamPos;
		PPU.IRQHBeamPos = (PPU.IRQHBeamPos & 0xFF) | ((*ptr & 1) << 8);

		if (PPU.HTimerEnabled && PPU.IRQHBeamPos != d)
			setHIRQ ();
		}
		break;

	case 0x4209:
	{
		int d = PPU.IRQVBeamPos;
		PPU.IRQVBeamPos = (PPU.IRQVBeamPos & 0xFF00) | *ptr;

		if (PPU.VTimerEnabled && PPU.IRQVBeamPos != d)
		{
			if (PPU.HTimerEnabled)
				setHIRQ ();
			else
			{
				if (PPU.IRQVBeamPos == SCHEDULER.screen_scanline)
				{
					CPU.irq = TRUE;
					check_for_IRQ();
				}

			}
		}
	}	break;

	case 0x420A:
	{
		int d = PPU.IRQVBeamPos;
		PPU.IRQVBeamPos = (PPU.IRQVBeamPos & 0xFF) | ((*ptr & 1) << 8);

		if (PPU.VTimerEnabled && PPU.IRQVBeamPos != d)
		{
			if (PPU.HTimerEnabled)
				setHIRQ ();
			else
			{
				if (PPU.IRQVBeamPos == SCHEDULER.screen_scanline)
				{
					CPU.irq = TRUE;
					check_for_IRQ();
				}

			}
		}
	}	break;

	case 0x420B:{	
			//enable channels not already used by HDMA
			byte channels_enable = *REG420B & ~(*REG420C);
                                                                                    
			if( channels_enable & 1)   startDMA(0x00);
			if( channels_enable & 2)   startDMA(0x10);
			if( channels_enable & 4)   startDMA(0x20);
			if( channels_enable & 8)   startDMA(0x30);
			if( channels_enable & 16)  startDMA(0x40);
			if( channels_enable & 32)  startDMA(0x50);
			if( channels_enable & 64)  startDMA(0x60);
			if( channels_enable & 128) startDMA(0x70);

			*REG420B = 0;//reset all DMA channels
		}
		break;

	case 0x420C:
		DEBUG("\nWrite %d to HDMA reg 0x420C at scanline %d",*REG420C,SCHEDULER.screen_scanline);
		break;
	case 0x420D:

	        if ( snes_settings.is_fastrom && (*ptr & 1) ) { // fastrom

			SCHEDULER.fastrom_cycles = SCHEDULER.fastrom_cycles_per_scanline;

			DEBUG("\nNow fastrom speed selected for bank 0x80-FF");
		}
		else//slowrom
		{
			SCHEDULER.fastrom_cycles = SCHEDULER.slowrom_cycles_per_scanline;

			DEBUG("\nNow slowrom speed selected for bank 0x80-FF");
		}
		break;

	case 0x420e:
	case 0x420f:
		// --->>> Unknown
		DEBUG("\nWrote unknown register %x",ptr-registers+0x2000);
		break;

	case 0x4210:
		// NMI ocurred flag (reset on read or write)
		PPU.reg4210 = MAX_5A22_VERSION;
		break;

	case 0x4211:
		// IRQ ocurred flag (reset on read or write)
		CPU.irq = FALSE;
		break;

	case 0x4212:  
			// v-blank, h-blank and joypad being scanned flags (read-only)
	case 0x4213:
			// I/O Port (read-only)
	case 0x4214:
	case 0x4215:
			// Quotent of divide (read-only)
	case 0x4216:
	case 0x4217:
			// Multiply product (read-only)
	case 0x4218:
	case 0x4219:
	case 0x421a:
	case 0x421b:
	case 0x421c:
	case 0x421d:
	case 0x421e:
	case 0x421f:
			// Joypad values (read-only)
		DEBUG("\n!!!Wrote read only register %x",ptr-registers+0x2000);
		break;

	case 0x4300:
	case 0x4310:
	case 0x4320:
	case 0x4330:
	case 0x4340:
	case 0x4350:
	case 0x4360:
	case 0x4370:
	  	break;

	case 0x4301:
	case 0x4311:
	case 0x4321:
	case 0x4331:
	case 0x4341:
	case 0x4351:
	case 0x4361:
	case 0x4371:
	  	break;

	case 0x4302:
	case 0x4312:
	case 0x4322:
	case 0x4332:
	case 0x4342:
	case 0x4352:
	case 0x4362:
	case 0x4372:
	  	break;

	case 0x4303:
	case 0x4313:
	case 0x4323:
	case 0x4333:
	case 0x4343:
	case 0x4353:
	case 0x4363:
	case 0x4373:
	  	break;

	case 0x4304:
	case 0x4314:
	case 0x4324:
	case 0x4334:
	case 0x4344:
	case 0x4354:
	case 0x4364:
	case 0x4374:
	 	break;

	case 0x4305:
	case 0x4315:
	case 0x4325:
	case 0x4335:
	case 0x4345:
	case 0x4355:
	case 0x4365:
	case 0x4375:
		break;

	case 0x4306:
	case 0x4316:
	case 0x4326:
	case 0x4336:
	case 0x4346:
	case 0x4356:
	case 0x4366:
	case 0x4376:
		break;

	case 0x4307:
	case 0x4317:
	case 0x4327:
	case 0x4337:
	case 0x4347:
	case 0x4357:
	case 0x4367:
	case 0x4377:
	 	break;

	case 0x4308:
	case 0x4318:
	case 0x4328:
	case 0x4338:
	case 0x4348:
	case 0x4358:
	case 0x4368:
	case 0x4378:
		break;

	case 0x4309:
	case 0x4319:
	case 0x4329:
	case 0x4339:
	case 0x4349:
	case 0x4359:
	case 0x4369:
	case 0x4379:
	 	break;


	case 0x430A:	HDMA[0].continuous = ( *REG430A(0) & 0x80 ) ? TRUE : FALSE;
			*REG430A(0) &= 0x7f;
			break;
	case 0x431A:	HDMA[1].continuous = ( *REG430A(1) & 0x80 ) ? TRUE : FALSE;
			*REG430A(1) &= 0x7f;
			break;
	case 0x432A:    HDMA[2].continuous = ( *REG430A(2) & 0x80 ) ? TRUE : FALSE;
			*REG430A(2) &= 0x7f;
			break;
	case 0x433A:    HDMA[3].continuous = ( *REG430A(3) & 0x80 ) ? TRUE : FALSE;
			*REG430A(3) &= 0x7f;
			break;
	case 0x434A:    HDMA[4].continuous = ( *REG430A(4) & 0x80 ) ? TRUE : FALSE;
			*REG430A(4) &= 0x7f;
			break;
	case 0x435A:    HDMA[5].continuous = ( *REG430A(5) & 0x80 ) ? TRUE : FALSE;
			*REG430A(5) &= 0x7f;
			break;
	case 0x436A:    HDMA[6].continuous = ( *REG430A(6) & 0x80 ) ? TRUE : FALSE;
			*REG430A(6) &= 0x7f;
			break;
	case 0x437A:    HDMA[7].continuous = ( *REG430A(7) & 0x80 ) ? TRUE : FALSE;
			*REG430A(7) &= 0x7f;
			break;

#ifndef ALLOWROMWRITE
	default:

		/*
	  	 If a rom write occurred, restore the original value contained in the backup copy
	 	 of the rom
		*/
		if( ptr<registers ){
				DEBUG("\n!!!ROM write prevented at rom address %x",ptr-rom);
				*ptr = backup_rom[ptr-rom];
		}
		break;
#endif

	}

	if (wordwrite)
		trapregwrite (ptr + 1, false);

	return ptr;
}
