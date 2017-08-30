

#ifndef MMIO_H
#define MMIO_H

/*It must be call before running the emulator.*/
extern void resetmemory();
          
/*print in the stdout the memory mapping used for the current rom. showMemoryMap() has to be already called*/
extern void showMemoryMap(void);

/*call this when the snes need to update joypads registers*/
void updatejoypads(void);

#define SNESMEM(addr) ( startaddr [(addr) >> 13] + (addr) )
/*
    For quick address acces.  To access a 65c816 address bb.aaaa, stored in
    variable addr.
    This macro return the pointer to the appropriate memory referred by
    the snes address passed as parameter.
    You can use this macro if no memory mapped I/O registers are accessed.
    Otherwise use TRAPREAD after SNESMEM or TRAPWRITE.
*/


#define TRAPREAD(ptr) ( ptr<registers ? ptr: trapregread( ptr, ISWORDOP) )
/*
 TRAPREAD is used to read the memory mapped I/O registers.
 ptr is the value returned from SNESMEM. If you are sure the address isn't in the
 memory mapped I/O area you can avoid call TRAPREAD.
*/



#ifdef ALLOWROMWRITE
		#define TRAPWRITE(ptr) if ( ptr>=registers ) trapregwrite(ptr,ISWORDOP)
#else
	 	#define TRAPWRITE(ptr) if ( ptr>=rom ) trapregwrite(ptr,ISWORDOP)
#endif
/*
 TRAPWRITE is used to write in memory mapped I/O registers.
 TRAPWRITE should be called after the write and before any flags get set
 based on the new value. ptr is returned from SNESMEM
*/


/*
 don't need to use this directly. use TRAPREAD instead
 except you are sure the addr is in registers area
*/
extern byte* trapregread (byte* addr, boolean wordread);
/*
  don't need to use this directly. use TRAPWRITE instead
  except you are sure the addr is in registers area
*/
extern byte* trapregwrite (byte* addr, boolean wordread);


/*****************************************************************************/
/************************pointers to all the snes memory**********************/
/*****************************************************************************/
 
extern  byte snes_memory[];

#define ram 	((byte*) snes_memory)				/*cpu ram*/
#define sram 	((byte*) &snes_memory[0x20000])			/*rom back up memory*/
#define expram	((byte*) &snes_memory[0x28000])			/*expantion ram*/
#define rom	((byte*) &snes_memory[0x38000])			/*the game code & data*/
#define registers	((byte*) &snes_memory[0x638000])		/*memory mapped i/o registers*/

extern byte *(startaddr []);
	/*  For quick address aaccess.  To access a 65c816 address bb.aaaa, stored in
 		variable addr:
		startaddr [addr >> 13] + (addr & 0x1FFF)
		This array is filled by set_addresses_mapping_table ().
	*/


/**************************************************************************/
/****************************I/O REGISTERS*********************************/
/**************************************************************************/

#define REG213F ((byte*)(registers + 0x013F)) /*read-only*/
	// PPU status flag/version number b7:field# in int.mode b6:ext.signal b4:1=PAL b0-3:version

/*JOYPADS REGISTERS*********************************************************/	
#define REG4218 	((byte*)(registers + 0x2218))	
#define REG4219		((byte*)(registers + 0x2219))	
#define REG421A	((byte*)(registers + 0x221A))	
#define REG421B	((byte*)(registers + 0x221B))	
#define REG421C	((byte*)(registers + 0x221C))	
#define REG421D	((byte*)(registers + 0x221D))	
#define REG421E	((byte*)(registers + 0x221E))	
#define REG421F	((byte*)(registers + 0x221F))	

/*MISC AND MATH REGISTERS**************************************************/
#define REG4202 ((byte*)(registers + 0x2202))
	// Multiplicand 'A'--8-bit (A is set first)
#define REG4203 ((byte*)(registers + 0x2203))
	// Multiplier 'B'--8-bit (B is set last)
#define REG4204 ((byte*)(registers + 0x2204))
#define REG4205 ((byte*)(registers + 0x2205))
	// Dividend 'C'--16-bit
#define REG4206 ((byte*)(registers + 0x2206))
	// Divisor-'B'--8-bit
#define REG4213 ((byte*)(registers + 0x2213))
	// Programmable I/O Port (input)
#define REG4214 ((byte*)(registers + 0x2214))
#define REG4215 ((byte*)(registers + 0x2215))
	// 16-bit quotient
#define REG4216 ((byte*)(registers + 0x2216))
#define REG4217 ((byte*)(registers + 0x2217))
	// 16-bit multiplication result, or remainder

#define REG2134 ((byte*)(registers + 0x0134))
#define REG2135 ((byte*)(registers + 0x0135))
#define REG2136 ((byte*)(registers + 0x0136))
	 // 16bit x 8bit multiply read result.


/*DMA, HDMA REGISTERS******************************************************/
#define REG420B ((byte*)(registers + 0x220B))
	// Set bits to initiate DMA transfer
#define REG420C ((byte*)(registers + 0x220C))
	// Same as $420B, but to enable automatic HDMA transfers
#define REG4300(chan) ((byte*)(registers + 0x2300 + chan))
	// DMA Control Register b7:(DMA)0=CPU->PPU b6:(HDMA)0=Absolute 1=Indirect addressing
	// b4:0=Inc/Dec Addresses 1=Fixed Address b3:0=Increment 1=Decrement b0-2:transfer mode

#define REG4301(chan) ((byte*)(registers + 0x2301 + chan))
	// $21xx address
#define REG4302(chan) ((dword*)(registers + 0x2302 + chan))
	// 24-bit CPU address
#define REG4305(chan) ((word*)(registers + 0x2305 + chan))
	// Bytes to transfer (DMA) or some kind of 24-bit CPU address (HDMA)
#define REG4307(chan) ((byte*)(registers + 0x2307 + chan))
#define REG4308(chan) ((word*)(registers + 0x2308 + chan))
#define REG430A(chan) ((byte*)(registers + 0x230A + chan))
	// don't know what 4307+ are for
#define REG ((byte*)(registers + 0x))


#endif//end of library
