

#include "general.h"
#include "mmio.h"
#include "cpu.h"
#include "dma.h"
#include "ppu.h"

		/*CPU Registers definition*/
typedef union
{
    struct { byte l,h; } B;
    word W;
#ifdef REGS_ALIGNED
    dword dummy;/*use to force 32bit memory alignament*/
#endif
} pair;

struct {
	union {
		dword complete;// 24-bit complete istruction address
		struct {
  			word PC;  //real program counter
			byte PBR; // Program Bank        - Selects bank of opcode fetched
			byte PBR_dummy;// Must be 0 to work
		}component;
	}pc;
	union {
		dword shifted;
		struct {
			byte DBR_dummy1; // Must be 0 to work
			byte DBR_dummy2; // Must be 0 to work
			byte DBR; // Data Bank        - Selects bank of data access (certainaddr.modes)
			byte DBR_dummy3; // Must be 0 to work
		}byte;
	}db;
	pair s;// Stack pointer
	pair y;// Y index register
	pair x;// X index register
	pair a;// Accumulator
	pair d;// D direct register

#ifdef PFAST
	dword p[8];
			/*processor status flags*/
	#define CARRY		reg.p[0]
	#define ZERO		reg.p[1]
	#define INT_DISABLE 	reg.p[2]
	#define BCD 		reg.p[3]
	#define INDEX		reg.p[4]
	#define MEMORY		reg.p[5]
	#define OVERFLOW	reg.p[6]
	#define NEGATIVE	reg.p[7]


	#define clear(f) 	((f) = 0)
	#define set(f)   	((f) = 1)
	#define check(f)  	(f)
	#define getStatusRegister()    ((byte) (CARRY?1:0) | (ZERO?2:0) | (INT_DISABLE?4:0) \
					| (BCD?8:0) | (INDEX?16:0) | (MEMORY?32:0) | (OVERFLOW?64:0) \
					| (NEGATIVE?128:0) )
	#define setStatusRegister(p)   { \
					 byte b = (p);\
					 CARRY=b&1; ZERO=b&2; INT_DISABLE=b&4; BCD=b&8;\
					 INDEX=b&16; MEMORY=b&32; OVERFLOW=b&64; NEGATIVE=b&128;\
					}

#else

	byte p; // Status register  - i.e. Flags
		// 7 6 5 4 3 2 1 0    Negative, oVerflow, 16-bit accuMulator, 16-bit
		// N V M X D I Z C    indeX, bcD mode, Interrupt Disable, Zero, Carry
		// Note: if E=1, X is the B(Hardware Break?) flag, and M is always 1
	#define P reg.p

	#define CARRY 		1
	#define ZERO 		2
	#define INT_DISABLE 	4
	#define BCD 		8
	#define INDEX 		16
	#define MEMORY 		32
	#define OVERFLOW 	64
	#define NEGATIVE 	128

	#define clear(f) 	(reg.p &= ~(f))
	#define set(f)   	(reg.p |=  (f))
	#define check(f)  	(reg.p & (f))
	#define getStatusRegister()	reg.p
	#define setStatusRegister(b)	reg.p = (b)

#endif
	boolean e; // Emulation mode flag

}reg;

#define E 		reg.e
#define S 		reg.s.W		//stack
#define D 		reg.d.W		//direct
#define DBR 		reg.db.byte.DBR	//data bank
#define DBRSHIFTED 	reg.db.shifted	//unsigned long with DBR as third byte
#define OPADDRESS	reg.pc.complete	//complete istruction address( PBR + PC )
#define PC		reg.pc.component.PC//pc
#define	PBR		reg.pc.component.PBR//program bank
#define ACCUM_16BIT 	reg.a.W		//16 bit accumulator
#define INDEX_X_16BIT 	reg.x.W		//16 bit x
#define INDEX_Y_16BIT 	reg.y.W		//16 bit y


/*
 *****************Flag setting macros used by the opcodes********************
*/
#define SAVE(A) (oldA = (A))
#define ADCSETCARRY(_tmpc) ((_tmpc) & CARRY_BIT ? set(CARRY) : clear(CARRY) )
#define SBCSETCARRY(_tmpc) ((_tmpc) & CARRY_BIT ? clear(CARRY) : set(CARRY) )
#define SETZERO(A) ((A) == 0 ? set(ZERO) : clear(ZERO) )
#define SETNEGATIVE(A) ( ((A) & SIGN_BIT) ? set(NEGATIVE) : clear(NEGATIVE) )
#define SETNEGATIVEXY(XY) ( ((XY) & SIGN_BIT_XY) ? set(NEGATIVE) : clear(NEGATIVE) )
#define SETOVERFLOW(A,oldA) ( ((A ^ oldA) & SIGN_BIT) && ((A>oldA?A-oldA:oldA-A)\
 			<= SIGN_BIT) ? set(OVERFLOW) : clear(OVERFLOW) )
	// For an overflow condition:
	// - The signs have to change, and
	// - The value of (greater - smaller) is <= 32768
#ifdef PFAST
	/*
	#undef	ADCSETCARRY
	#define ADCSETCARRY(_tmpc) ( CARRY = (_tmpc) & CARRY_BIT )
	#undef	SBCSETCARRY
	#define SBCSETCARRY(_tmpc) ( CARRY = !( (_tmpc) & CARRY_BIT ) )
	You can also turn on this optimization but then you have to
	change the opcodes that use carry like ADC because this opcodes
	add the carry flag in the addition, being the carry flag equal to 1 or 0
	Whit this optimization you have to test the value of carry and then
	add 1 if it is needed
	*/
	#undef	SETZERO
	#define SETZERO(A) ( ZERO = !(A) )
	#undef	SETNEGATIVE
	#define SETNEGATIVE(A) ( NEGATIVE = (A) & SIGN_BIT )
	#undef 	SETNEGATIVEXY
	#define SETNEGATIVEXY(XY) ( NEGATIVE = (XY) & SIGN_BIT_XY )
	#undef 	SETOVERFLOW
	#define SETOVERFLOW(A,oldA) ( OVERFLOW = ( (A ^ oldA) & SIGN_BIT) && ((A>oldA ? A-oldA:oldA-A)\
 			<= SIGN_BIT) )
#endif

/*
 **************************ADDRESSING MODE************************
 Using these returns a pointer to the appropriate memory address.
 The PC relative modes aren't listed since what is contained at the
 new location is irrelevant to the current instruction and they are
 use in branchs and jumps
*/

/*opdata is the operand of the current opcode. see the run() function*/
#define OPBYTE ((byte) opdata)
#define OPWORD ((word) opdata)
#define OPLONG opdata


#define IMM_BYTE OPBYTE
	// OPC %#b

#define IMM_ACCUM ((ATYPE) opdata)
	// OPA %#

#define IMM_INDEX ((XYTYPE) opdata)
	// OPX %#i

#define ABS ( tmp = DBRSHIFTED + OPWORD, SNESMEM (tmp ) )
	// OPC %w

#define ABS_LONG (SNESMEM (OPLONG))
	// OPC $%L

#define DIR ( tmp = (D + OPBYTE), SNESMEM( (word)tmp ) )
	// OPC <%b


#define DIR_INDIR_INDEX_Y ( tmp = *(word*)SNESMEM( (word)(D + OPBYTE) ) + \
 				DBRSHIFTED + INDEX_Y, SNESMEM(tmp) )
	// OPC (<%b),Y
	
#define DIR_INDIR_LONG_INDEX_Y ( tmp =\
		( (*(dword*)SNESMEM( (word)(D+OPBYTE ) ) ) & 0xFFFFFF)\
				+ INDEX_Y, SNESMEM (tmp) )
// OPC [<%b],Y



#define DIR_INDEX_INDIR_X ( tmp = *(word *)SNESMEM( (word)(D+OPBYTE + INDEX_X) )\
		 + DBRSHIFTED, SNESMEM (tmp) )
	 // OPC (<%b,X)

#define DIR_INDEX_X ( tmp = (D + OPBYTE + INDEX_X), SNESMEM( (word)tmp ) )
	// OPC <%b,X

#define DIR_INDEX_Y ( tmp = (D + OPBYTE + INDEX_Y) , SNESMEM( (word)tmp ) )
	// OPC <%b,Y

#define ABS_INDEX_X ( tmp = DBRSHIFTED + OPWORD + INDEX_X, SNESMEM(tmp) )
	// OPC %w,X

#define ABS_INDEX_Y ( tmp = DBRSHIFTED + OPWORD + INDEX_Y, SNESMEM(tmp) )
	// OPC %w, Y

#define ABS_LONG_INDEX_X ( tmp = (OPLONG + INDEX_X) & 0xFFFFFF, SNESMEM(tmp) )
	// OPC %L,X

#define ABS_INDIR ( SNESMEM (OPWORD) )
	// JML (%w) or JMP (%w)

#define DIR_INDIR ( tmp = *(word*)SNESMEM( (word)(D + OPBYTE) )\
 			+ DBRSHIFTED, SNESMEM (tmp) )
	// OPC (<%b)

#define DIR_INDIR_LONG ( tmp = (*(dword*)SNESMEM( (word)(D + OPBYTE) ) )\
				 & 0xFFFFFF, SNESMEM (tmp) )
	// OPC [<%b]



#define ABS_INDEX_INDIR_X_JMP ( PC = OPWORD + INDEX_X, *(word*)SNESMEM(OPADDRESS) )
	// JMP (%w,X) - returns the new value for the PC.  As evidenced by
	// Tetris Attack, the PBR (not DBR) should be used for both bank values.
        /*qwerty version: (tmp = OPWORD + INDEX_X + (PC & 0xFF0000),\
			 *(word*)SNESMEM(tmp) | (PC & 0xFF0000) )*/

#define S_REL (tmp = (S + OPBYTE),SNESMEM ( (word)tmp ) )
	// OPC <%b,S


#define S_REL_INDIR_INDEX_Y ( tmp = (*(word*)SNESMEM ( (word)(S + OPBYTE) ) \
		+ DBRSHIFTED + INDEX_Y ) & 0xffffff, SNESMEM (tmp) )
	// OPC (<%b,S),Y

/*
 This macros are used to push and pull data onto the stack
*/
#ifdef PUSHPULLFAST
	#define PUSHBYTE(b) ( *(byte*)SNESMEM(S) = (byte)(b), S-- )
	#define PUSHWORD(w) ( *(word*)SNESMEM(S-1) = (word)(w), S -= 2 )

	#define PULLBYTE(db) ( S++, (db) = *(byte*)SNESMEM(S) )
	#define PULLWORD(dw) ( S+= 2, (dw) = *(word*)SNESMEM(S-1) )
#else
	byte * t_mem;
	#define PUSHBYTE(b) { t_mem = SNESMEM(S); *(byte*)t_mem = (byte)(b);\
	 		      if ( t_mem>=rom ) trapregwrite(t_mem,0); S--; }
	#define PUSHWORD(w) { t_mem = SNESMEM(S-1); *(word*)t_mem = (word)(w);\
			      if ( t_mem>= rom ) trapregwrite(t_mem,1); S -= 2;}

	#define PULLBYTE(db) { S++; t_mem = SNESMEM(S);\
	 		   db = *(byte*)( (t_mem<registers) ? t_mem: trapregread( t_mem, 0) ); }
	#define PULLWORD(dw) { S+= 2;  t_mem = SNESMEM(S-1);\
			   dw = *(word*)( (t_mem<registers) ? t_mem: trapregread( t_mem, 1) ); }
#endif
/*
 BCD: Binary Coded Decimal
 Every nibble is considered as a decimal digit. So for example you can
 code 42(decimal) and 17(decimal) in this way:
		  4  2 					    1  7
  42 decimal = 0100 0010(BCD)     and      17(decimal) = 0001 0111(BCD)
  Only ten values of the sixteen possible for a nibble is used, but it is
 usefull
  for devices that must convert number in strings.
  If you want to add togheter two number coded BCD then, for every nibble, you
  must check if the number is greater than 9 and in that case you must add six
  in the way to skip the unused values of nibble and add the carry to the next
*/
#define bcdaddadjust_byte(A)\
	{\
		if ((A & 15) > 9) A += 6;\
		if (((A >> 4) & 15) > 9) A += (6 << 4);\
	}
#define bcdsubadjust_byte(A)\
	{\
		if ((A & 15) > 9) A -= 6;\
		if (((A >> 4) & 15) > 9) A -= (6 << 4);\
	}
#define bcdaddadjust_word(A)\
	{\
		if ((A & 15) > 9) A += 6;\
		if (((A >> 4) & 15) > 9) A += (6 << 4);\
		if (((A >> 8) & 15) > 9) A += (6 << 8);\
		if (((A >> 12) & 15) > 9) A += (6 << 12);\
	}
#define bcdsubadjust_word(A)\
	{\
		if ((A & 15) > 9) A -= 6;\
		if (((A >> 4) & 15) > 9) A -= (6 << 4);\
		if (((A >> 8) & 15) > 9) A -= (6 << 8);\
		if (((A >> 12) & 15) > 9) A -= (6 << 12);\
	}

struct cpu CPU;

void resetcpu (void)
{
		/*force to 0 all the dummy register variables*/
	reg.pc.complete = 0;
	reg.db.shifted = 0;

	S = 0x1FF;
	D =  0;
	PBR = 0;// (curheader.fastrom ? 0x80 : 0);
	DBR = 0;
	INDEX_X_16BIT &= 0xFF;
	INDEX_Y_16BIT &= 0xFF;

	E =1;
	set(MEMORY);
	set(INDEX);
	clear(BCD);
	set(INT_DISABLE);

	PC = rom_header->resetvector;

	CPU.irq = FALSE;
	CPU.WaitingForInterrupt = FALSE;
	CPU.branch_skip = FALSE;//used for sound skip
}



static void doIRQ(void)       
{
               
 	if (CPU.WaitingForInterrupt)
	{
		CPU.WaitingForInterrupt = FALSE;
		PC++;
	}

	if( E ){

		PUSHWORD (PC);
		PUSHBYTE (getStatusRegister());
		set(INT_DISABLE);
		clear(BCD);
		OPADDRESS = *(word *)SNESMEM (0x00FFFE);//it should set PBR to 0
	}else{
		PUSHBYTE (PBR);
		PUSHWORD (PC);
		PUSHBYTE (getStatusRegister());
		set(INT_DISABLE);
		clear(BCD);
		OPADDRESS = *(word *)SNESMEM (0x00FFEE);//it should set PBR to 0
	}

	SCHEDULER.cycles-=8;
}

#define CHECK_FOR_IRQ()	 if ( CPU.irq && !check(INT_DISABLE)  )   doIRQ()

void check_for_IRQ(void){
                       CHECK_FOR_IRQ();
}


static void doNMI(void)
{
	if (CPU.WaitingForInterrupt)
	{
		CPU.WaitingForInterrupt = FALSE;
		PC++;
    	}

	if( E ){
		PUSHWORD (PC);
		PUSHBYTE (getStatusRegister());
		set(INT_DISABLE);
		clear(BCD);
		OPADDRESS = *(word *)SNESMEM (0x00FFFA);//it should set PBR to 0
	}else{
		PUSHBYTE (PBR);
		PUSHWORD (PC);
		PUSHBYTE (getStatusRegister());
		set(INT_DISABLE);
		clear(BCD);
		OPADDRESS = *(word *)SNESMEM (0x00FFEA);//it should set PBR to 0
	}

	SCHEDULER.cycles-=8;
}

static void hardwareHandling(void);

static void check_speed(void);
  
/*
 function used to starting emulation.
 A rom must be already loaded and resetsystem function called
*/

void run(){
	byte opcode;/*current opcode*/
	dword opdata;/*current opcode data*/
	register dword tmp, _tmpc;/*used by addressing mode macros*/
	byte *mem; byte *mem2;/*temporary value used by opcode*/

check_mode://check the curret mode (native or emulation) and then the memory/index flag
	   //and go to the appropriate routine. There is a routine to
	   //handle the different working mode of the processor, one for every
	   //combination of the flag m and x, plus the routine for emulation mode

	if( E ) goto emulation_mode; //emulation mode

	else { //native mode
check_mx:
	     if( check(INDEX) ){       //index registers 8 bit

		if( check(MEMORY) ) goto m1_x1;//accumulator/memory 8 bit
		else goto m0_x1;               //accumulator/memory 16 bit

	     }else{                    //index registers 16 bit

		if( check(MEMORY) ) goto m1_x0;//accumulator/memory 8 bit
		else goto m0_x0;               //accumulator/memory 16 bit

	     }
	}

m0_x0:

#define A_16BIT
#define XY_16BIT
#include "cpuexec.c"    /*include the routine that handle the m=0 x=0 cpu state*/


m0_x1:

#undef XY_16BIT
#include "cpuexec.c"	/*include the routine that handle the m=0 x=1 cpu state*/


m1_x1:

#undef A_16BIT
#include "cpuexec.c"	/*include the routine that handle the m=1 x=1 cpu state*/


m1_x0:

#define XY_16BIT
#include "cpuexec.c"	/*include the routine that handle the m=1 x=0 cpu state*/


emulation_mode:

#undef XY_16BIT
#define EMU_MODE

#undef PUSHBYTE
#undef PUSHWORD
#undef PULLBYTE
#undef PULLWORD
/*I must rewrite these macros to be sure the stack wrap on page 0x01 in emulation mode*/
#define PUSHBYTE(b) ( *(byte*)SNESMEM(S) = (byte)(b), reg.s.B.l-- )
#define PUSHWORD(w) ( *(word*)SNESMEM(S-1) = (word)(w), reg.s.B.l -= 2 )
#define PULLBYTE(db) ( reg.s.B.l++, (db) = *(byte*)SNESMEM(S) )
#define PULLWORD(dw) ( reg.s.B.l+= 2, (dw) = *(word*)SNESMEM(S-1) )

#include "cpuexec.c"	/*include the routine that handle the emulation mode cpu state*/
#undef EMU_MODE

}

struct scheduler SCHEDULER;


void resetscheduler(void)
{

 int cycles_per_frame=0;

// we have to decide how many cpu cycles do per scanline.
// slowrom_cycles_per_scanline tell us the number of cycles in the case of slowrom or
// in the case of fastrom that execute code in banks 0x00-0x80.
// fastrom_cycles_per_scanline tell us the number of cycles in the case of fastrom that
// execute code in banks 0x80-0xFF when 0x420D is set.

 cycles_per_frame = SLOWROM_SPEED / snes_settings.tv_hz;

 SCHEDULER.slowrom_cycles_per_scanline = cycles_per_frame / snes_settings.max_vcounter;

 cycles_per_frame = FASTROM_SPEED / snes_settings.tv_hz;

 SCHEDULER.fastrom_cycles_per_scanline = cycles_per_frame / snes_settings.max_vcounter;

 //cycles_per_scanline mantain the current value of cycles per scanline. Since
 //the game speed can change depending on register 0x420D cycles_per_scanline will
 //be equal to slowrom_cycles_per_scanline or fastrom_cycles_per_scanline.
 //I think the rom start with slowrom speed because the value of program bank
 //register is 0
 SCHEDULER.cycles_per_scanline =  SCHEDULER.slowrom_cycles_per_scanline;

 //fastrom_cycles mantain the current speed (setted by register 0x420D ) for
 //banks 80-FF. when the emulator start reg 0x420D is not set. So in banks
 //80-FF the game run in slowrom speed.
 SCHEDULER.fastrom_cycles = SCHEDULER.slowrom_cycles_per_scanline;

 SCHEDULER.cycles =  SCHEDULER.cycles_per_scanline;
 SCHEDULER.screen_scanline = 0;
 SCHEDULER.next_event = 0;
 SCHEDULER.which_event = END_SCANLINE;

 SCHEDULER.fastromHBlankStart = SCHEDULER.fastrom_cycles_per_scanline - 
				((256 * SCHEDULER.fastrom_cycles_per_scanline) / snes_settings.max_hcounter);
 SCHEDULER.slowromHBlankStart = SCHEDULER.slowrom_cycles_per_scanline -
				((256 * SCHEDULER.slowrom_cycles_per_scanline) / snes_settings.max_hcounter);

}


/*
 when cycles reach the next_event value, hardwareHandling() function must be called
 to emulate the appropriate hardware.
*/
static void hardwareHandling(void)
{

 switch( SCHEDULER.which_event ){

 case END_SCANLINE:

	if( hdma_inprogress &&  SCHEDULER.screen_scanline <= PPU.screen_height)
		dohdmaline ();


	if ( ++SCHEDULER.screen_scanline >= snes_settings.max_vcounter)
	{
	    //starting of new frame here
	    SCHEDULER.screen_scanline = 0;
	    resethdma ();// Restart HDMA here
	}
                                          
	if ( SCHEDULER.screen_scanline == FIRST_VISIBLE_LINE)
	{
	     //first visible line of the snes here
	     PPU.reg4210 = MAX_5A22_VERSION;
	     StartScreenRefresh ();
	}


	if ( SCHEDULER.screen_scanline == PPU.screen_height + FIRST_VISIBLE_LINE )
	{
	     // Start of V-blank here
	     EndScreenRefresh ();
	     hdma_inprogress = 0;//to avoid the dohdmaline call untill the end of vblank
	     
	     if( !PPU.ForcedBlanking )
	     {
		PPU.OAMAddr = PPU.SavedOAMAddr;
		PPU.OAMFlip = 0;
		PPU.FirstSprite = 0;
		if(PPU.OAMPriorityRotation)
			PPU.FirstSprite = PPU.OAMAddr>>1;
	     }

	    if (  PPU.NMIEnable & !(PPU.reg4210 & 0x80) ) //start NMI
	    {
                             doNMI();
	     }
	     PPU.reg4210 = 0x80 | MAX_5A22_VERSION;
	     	     
       	}

	if (PPU.VTimerEnabled && !PPU.HTimerEnabled &&
	    SCHEDULER.screen_scanline == PPU.IRQVBeamPos)
	{
	    //V-IRQ
	    CPU.irq = TRUE;
	    CHECK_FOR_IRQ();
	}

	if ( SCHEDULER.screen_scanline == PPU.screen_height + 3 )
	{
	      //update joypads here
	      updatejoypads();
	}

	if (  SCHEDULER.screen_scanline >= FIRST_VISIBLE_LINE &&
	      SCHEDULER.screen_scanline < PPU.screen_height + FIRST_VISIBLE_LINE)
	{
	     //render line here
                  RenderLine (SCHEDULER.screen_scanline - FIRST_VISIBLE_LINE);
	}
           			 /**********************************/
			/*ready to execute others istructions*/
		             /**********************************/
	SCHEDULER.cycles += SCHEDULER.cycles_per_scanline;

			/******************************************/
			/*RESCHEDULING the next event to be done*/
			/*****************************************/
	if ( PPU.HTimerEnabled &&
	   (!PPU.VTimerEnabled || (SCHEDULER.screen_scanline == PPU.IRQVBeamPos) ) &&
	   (PPU.HTimerPosition >= 0) )
	{
		SCHEDULER.next_event = PPU.HTimerPosition;
	    	SCHEDULER.which_event = IRQ_REQUIRED;
	}
	else
	{
		SCHEDULER.next_event = 0;
		SCHEDULER.which_event = END_SCANLINE;
	}

	break;

 case IRQ_REQUIRED:
	//H-IRQ

	if ( PPU.HTimerEnabled &&
	   (!PPU.VTimerEnabled || (SCHEDULER.screen_scanline == PPU.IRQVBeamPos) ) )
	{
	    CPU.irq = TRUE;
	    CHECK_FOR_IRQ();
	}

	SCHEDULER.next_event = 0;
	SCHEDULER.which_event = END_SCANLINE;

	break;
  }

}


void setHIRQ (void)
{
	if (PPU.HTimerEnabled)
	{
		PPU.HTimerPosition = SCHEDULER.cycles_per_scanline - (PPU.IRQHBeamPos *
					SCHEDULER.cycles_per_scanline / SNES_HCOUNTER_MAX);

		if ( PPU.HTimerPosition < 0 )
		{
			SCHEDULER.next_event = 0;
			SCHEDULER.which_event = END_SCANLINE;			
			DEBUG("\n                  !!!Hey,  H-IRQ setted outside scanline!!!");
		}

		if ( !PPU.VTimerEnabled || SCHEDULER.screen_scanline == PPU.IRQVBeamPos)
		{
			if (PPU.HTimerPosition > SCHEDULER.cycles)
			{
				// Missed the IRQ on this line already
				SCHEDULER.next_event = 0;
				SCHEDULER.which_event = END_SCANLINE;

			}
			else
			{
				SCHEDULER.next_event = PPU.HTimerPosition;
				SCHEDULER.which_event = IRQ_REQUIRED;
			}
		}
	}
}

/*
  when memory access speed change, set up new value for cycles per scanline
  and h-irq cycles count
*/
static void check_speed(void)
{

  if( PBR >= 0x80){//fastrom banks
      if( SCHEDULER.cycles_per_scanline != SCHEDULER.fastrom_cycles)
      {
	SCHEDULER.cycles = (SCHEDULER.cycles * SCHEDULER.fastrom_cycles)
			       / SCHEDULER.cycles_per_scanline;

	SCHEDULER.cycles_per_scanline = SCHEDULER.fastrom_cycles;

	PPU.HTimerPosition = (PPU.HTimerPosition * SCHEDULER.fastrom_cycles)
			       / SCHEDULER.cycles_per_scanline;
 
	SCHEDULER.next_event = (SCHEDULER.next_event * SCHEDULER.fastrom_cycles)
			       / SCHEDULER.cycles_per_scanline;
      }
   }
   else//slowrom speed
   {
      if( SCHEDULER.cycles_per_scanline != SCHEDULER.slowrom_cycles_per_scanline )
      {
	SCHEDULER.cycles = (SCHEDULER.cycles * SCHEDULER.slowrom_cycles_per_scanline)
			          / SCHEDULER.cycles_per_scanline;

	SCHEDULER.cycles_per_scanline = SCHEDULER.slowrom_cycles_per_scanline;

	PPU.HTimerPosition = (PPU.HTimerPosition * SCHEDULER.slowrom_cycles_per_scanline)
				  / SCHEDULER.cycles_per_scanline;

	SCHEDULER.next_event = (SCHEDULER.next_event * SCHEDULER.slowrom_cycles_per_scanline)
			       / SCHEDULER.cycles_per_scanline;
      }
   }

}

dword getPC(void)
{   /*
  int t;
  byte opcode = *(byte*) SNESMEM(OPADDRESS);

  for (t = 0; instrlist[t].mnemonic[0] != '\0'; t++)
  {
	if (instrlist[t].mnemonic[0] == opcodelist[opcode].format[0] &&
		instrlist[t].mnemonic[1] == opcodelist[opcode].format[1] &&
			instrlist[t].mnemonic[2] == opcodelist[opcode].format[2]) 
	{
		printf("         %s",instrlist[t].descr);
		break;
	}
  }    */
 return OPADDRESS;
}

#ifdef DEBUGGER

struct instructioninfo instrlist [] = {
	"ADC", "Add memory to accumulator with carry",
	"AND", "AND arg with accumulator (A&=arg)",
	"SHL", "(ASL) Shift left arg (C set to MSB)",
	"BCC", "Branch if Carry Clear",
	"BCS", "Branch if carry set",
	"BEQ", "Branch if zero (or branch if equal)",
	"BIT", "And A w/ arg, N=bit 7, V=bit 6, discard result",
	"BMI", "Branch if minus (if N=1)",
	"BNE", "Branch if not zero (branch if not equal)",
	"BPL", "Branch if plus (if N=0)",
	"BRA", "Branch always (Relative)",
	"BRK", "Break (Push PC+2; Call Interrupt *0.FFE6)",
	"BRL", "Branch Long (16-bit JMP)",
	"BVC", "Branch if Overflow Clear",
	"BVS", "Branch if overflow set",
	"CLC", "Clear the Cary Bit (P&~1)",
	"CLD", "Clear the BCD mode bit (P&~8)",
	"CLI", "Clear the Interrupt disable bit (P&~4)",
	"CLV", "Clear the overflow bit (P&~64)",
	"CMP", "Compare arg with A: A>=a?C=1:N=1,A=a?Z=1:Z=0",
	"CSP", "(COP) Call Sys.Proc. (Push PC; Call Int. *0.FFE4)",
	"CPX", "Compare arg with X",
	"CPY", "Compare arg with Y",
	"DEC", "Decrement arg (A or a memory location)",
	"DEX", "Decrement X",
	"DEY", "Decrement Y",
	"XOR", "(EOR) Exclusive OR A with arg (A^=arg)",
	"INC", "Increment arg (A or a memory location)",
	"INX", "Increment X",
	"INY", "Increment Y",
	"JML", "Jump Long (PC=16bit arg->24bit address)",
	"JMP", "Jump (PC=arg or PC=*arg)",
	"JSL", "Jump to Subroutine Long (PC=24bit arg)",
	"JSR", "Jump to Subroutine",
	"LDA", "Load A with a value",
	"LDX", "Load Index X with a value",
	"LDY", "Load Index Y with a value",
	"SHR", "(LSR) Shift Right (C set to LSB)",
	"MVN", "Move- (DBR=arg;while(A){*Y=*arg2.X;X++,Y++,A--})",
	"MVN", "Move+ (DBR=arg;while(A){*Y=*arg2.X;X--,Y--,A--})",
	"NOP", "No operation--waste two cycles",
	"ORA", "OR arg with accumulator (A|=arg)",
	"PSH", "(PE?,PH?) Push onto the stack",
	"PUL", "(PL?) Pull from the stack",
	"CLP", "(REP) Clear status bits in P",
	"ROL", "Rotate arg left (carry included)",
	"ROR", "Rotate arg right (carry included)",
	"RTI", "Return from interrupt & restore flags",
	"RTL", "Return from Subroutine Long",
	"RTS", "Return from Subroutine",
	"SBC", "Subtract from Accumulator with Carry (Borrow)",
	"SEC", "Set the Carry bit to 1",
	"SED", "Set the BCD mode bit to 1",
	"SEI", "Set the Interrupt Disable bit to 1",
	"SEP", "Set status bits in P",
	"HLT", "(STP-Stop the clock) Halt CPU until reset",
	"STA", "Store Accumulator in memory",
	"STX", "Store Index X in memory",
	"STY", "Store Index Y in memory",
	"CLR", "(STZ) Clear memory location",
	"TRB", "Test and reset bits",
	"TSB", "Test and set bits",
	"TAX", "Transfer (copy) A into X",
	"TXA", "Transfer (copy) X into A",
	"TAY", "Transfer (copy) A into Y",
	"TYA", "Transfer (copy) Y into A",
	"TAD", "Transfer (copy) A into Direct",
	"TDA", "Transfer (copy) Direct into A",
	"TAS", "Transfer (copy) A into Stack Pointer",
	"TSA", "Transfer (copy) Stack pointer into A",
	"TSX", "Transfer (copy) Stack pointer into X",
	"TXS", "Transfer (copy) X into Stack pointer",
	"TXY", "Transfer (copy) X into Y",
	"TYX", "Transfer (copy) Y into X",
	"WAI", "Wait for interrupt",
	"SWA", "(XBA) Swap 16-bit accumulator halves",
	"XCE", "Exchange the Carry and Emulation bits",
	"???", "UNASSIGNED OPCODE $42",
	"\0\0\0", "[Description search error]"
};

struct opcodeinfo opcodelist [] = {
	// 0x0
	"BRK", 2, 8,
	"ORA (<%b,X)", 2, 6,
	"CSP", 2, 8,
	"ORA <%b,s", 2, 4,
	"TSB <%b", 2, 5,
	"ORA <%b", 2, 3,
	"SHL <%b", 2, 5,
	"ORA [<%b]", 2, 6,
	// 0x8
	"PSH P", 1, 3, // PHP
	"ORA %#", 2, 2,
	"SHL A", 1, 2,
	"PSH D", 1, 4, // PHD Push Direct
	"TSB %w", 3, 6,
	"ORA %w", 3, 4,
	"SHL %w", 3, 6,
	"ORA %L", 4, 5,
	// 0x10
	"BPL @%b", 2, 2,
	"ORA (<%b),Y", 2, 5,
	"ORA (<%b)", 2, 5,
	"ORA (<%b,S),Y", 2, 7,
	"TRB <%b", 2, 5,
	"ORA <%b,X", 2, 4,
	"SHL <%b,X",2, 6,
	"ORA [<%b],Y", 2, 6,
	// 0x18
	"CLC", 1, 2,
	"ORA %w,Y", 3, 4,
	"INC A", 1, 2,
	"TAS", 1, 2,
	"TRB %w", 3, 6,
	"ORA %w,X", 3, 4,
	"SHL %w,X", 3, 7,
	"ORA %L,X", 4, 5,
	// 0x20
	"JSR %w", 3, 6,
	"AND (<%b,X)", 2, 6,
	"JSL %L", 4, 8,
	"AND <%b,s", 2, 4,
	"BIT <%b", 2, 3,
	"AND <%b", 2, 3,
	"ROL <%b", 2, 5,
	"AND [<%b]", 2, 6,
	// 0x28
	"PUL P", 1, 4, // PLP
	"AND %#", 2, 2,
	"ROL A", 1, 2,
	"PUL D", 1, 5, // PLD Pull Direct
	"BIT %w", 3, 4,
	"AND %w", 3, 4,
	"ROL %w", 3, 6,
	"AND %L", 4, 5,
	// 0x30
	"BMI @%b", 2, 2,
	"AND (<%b),Y", 2, 5,
	"AND (<%b)", 2, 5,
	"AND (<%b,s),Y", 2, 7,
	"BIT <%b,X", 2, 4,
	"AND <%b,X", 2, 4,
	"ROL <%b,X", 2, 6,
	"AND [<%b],Y", 2, 6,
	// 0x38
	"SEC", 1, 2,
	"AND %w,Y", 3, 4,
	"DEC A", 1, 2,
	"TSA", 1, 2,
	"BIT %w,X", 3, 4,
	"AND %w,X", 3, 4,
	"ROL %w,X", 3, 7,
	"AND %L,X", 4, 5,
	// 0x40
	"RTI", 1, 7,
	"XOR (<%b,X)", 2, 6,
	"???", 2, 2,
	"XOR <%b,s", 2, 4,
	"MVP XYA %w", 3, 7,
	"XOR <%b", 2, 3,
	"SHR <%b", 2, 5,
	"XOR [<%b]", 2, 6,
	// 0x48
	"PSH A", 1, 3, // PHA
	"XOR %#", 2, 2,
	"SHR A", 1, 2,
	"PSH PBR", 1, 3, // PHK
	"JMP %w", 3, 3,
	"XOR %w", 3, 4,
	"SHR %w", 3, 6,
	"XOR %L", 4, 5,
	// 0x50
	"BVC @%b", 2, 2,
	"XOR (<%b),Y", 2, 5,
	"XOR (<%b)", 2, 5,
	"XOR (<%b,s),Y", 2, 7,
	"MVN XYA %w", 3, 7,
	"XOR <%b,X", 2, 4,
	"SHR <%b,X", 2, 6,
	"XOR [<%b],Y", 2, 6,
	// 0x58
	"CLI", 1, 2,
	"XOR %w,Y", 3, 4,
	"PSH Y", 1, 3, // PHY
	"TAD", 1, 2,
	"JML %L", 4, 4,
	"XOR %w,X", 3, 4,
	"SHR %w,X", 3, 7,
	"XOR %L,X", 4, 5,
	// 0x60
	"RTS", 1, 6,
	"ADC (<%b,X)", 2, 6,
	"PSH @%w", 3, 6, // PER
	"ADC <%b,s", 2, 4,
	"CLR <%b", 2, 3,
	"ADC <%b", 2, 3,
	"ROR <%b", 2, 5,
	"ADC [<%b]", 2, 6,
	// 0x68
	"PUL A", 1, 4, // PLA
	"ADC %#", 2, 2,
	"ROR A", 1, 2,
	"RTL", 1, 6,
	"JMP (%w)", 3, 5,
	"ADC %w", 3, 4,
	"ROR %w", 3, 6,
	"ADC %L", 4, 5,
	// 0x70
	"BVS @%b", 2, 2,
	"ADC (<%b),Y", 2, 5,
	"ADC (<%b)", 2, 5,
	"ADC (<%b,s),Y", 2, 7,
	"CLR <%b,X", 2, 4,
	"ADC <%b,X", 2, 4,
	"ROR <%b,X", 2, 6,
	"ADC [<%b],Y", 2, 6,
	// 0x78
	"SEI", 1, 2,
	"ADC %w,Y", 3, 4,
	"PUL Y", 1, 4, // PLY
	"TDA", 1, 2,
	"JMP (%w,X)", 3, 6,
	"ADC %w,X", 3,4,
	"ROR %w,X", 3,7,
	"ADC %L,X",4,5,
	// 0x80
	"BRA @%b", 2, 2,
	"STA (<%b,X)", 2, 6,
	"BRL @%w", 3, 3,
	"STA <%b,S", 2, 4,
	"STY <%b", 2, 3,
	"STA <%b", 2, 3,
	"STX <%b", 2, 3,
	"STA [<%b]", 2, 6,
	// 0x88
	"DEY", 1, 2,
	"BIT %#", 2, 2,
	"TXA", 1, 2,
	"PSH DBR", 1, 3, // PHB
	"STY %w", 3, 4,
	"STA %w", 3, 4,
	"STX %w", 3, 4,
	"STA %L", 4, 5,
	// 0x90
	"BCC @%b", 2, 2,
	"STA (<%b),Y", 2, 6,
	"STA (<%b)", 2, 5,
	"STA (<%b,s),Y", 2, 7,
	"STY <%b,X", 2, 4,
	"STA <%b,X", 2, 4,
	"STX <%b,Y", 2, 4,
	"STA [<%b],Y", 2, 6,
	// 0x98
	"TYA", 1, 2,
	"STA %w,Y", 3, 5,
	"TXS", 1, 2,
	"TXY", 1, 2,
	"CLR %w", 3, 4,
	"STA %w,X", 3, 5,
	"CLR %w,X", 3, 5,
	"STA %L,X", 4, 5,
	// 0xA0
	"LDY %#i", 2, 2,
	"LDA (<%b,X)", 2, 6,
	"LDX %#i", 2, 2,
	"LDA <%b,S", 2, 4,
	"LDY <%b", 2, 3,
	"LDA <%b", 2, 3,
	"LDX <%b", 2, 3,
	"LDA [<%b]", 2, 6,
	// 0xA8
	"TAY", 1, 2,
	"LDA %#", 2, 2,
	"TAX", 1, 2,
	"PUL DBR", 1, 4, // PLB
	"LDY %w", 3, 4,
	"LDA %w", 3, 4,
	"LDX %w", 3, 4,
	"LDA %L", 4, 5,
	// 0xB0
	"BCS @%b", 2, 2,
	"LDA (<%b),Y", 2, 5,
	"LDA (<%b)", 2, 5,
	"LDA (<%b,S),Y", 2, 7,
	"LDY <%b,X", 2, 4,
	"LDA <%b,X", 2, 4,
	"LDX <%b,Y", 2, 4,
	"LDA [<%b],Y", 2, 6,
	// 0xB8
	"CLV", 1, 2,
	"LDA %w,Y", 3, 4,
	"TSX", 1, 2,
	"TYX", 1, 2,
	"LDY %w,X", 3, 4,
	"LDA %w,X", 3, 4,
	"LDX %w,Y", 3, 4,
	"LDA %L,X",4, 5,
	// 0xC0
	"CPY %#i", 2, 2,
	"CMP (<%b,X)", 2, 6,
	"CLP %#b", 2, 3,
	"CMP <%b,S", 2, 4,
	"CPY <%b", 2, 3,
	"CMP <%b", 2, 3,
	"DEC <%b", 2, 5,
	"CMP [<%b]", 2,6,
	// 0xC8
	"INY", 1, 2,
	"CMP %#", 2, 2,
	"DEX", 1, 2,
	"WAI", 1, 3,
	"CPY %w", 3, 4,
	"CMP %w", 3, 4,
	"DEC %w", 3, 6,
	"CMP %L", 4, 5,
	// 0xD0
	"BNE @%b", 2, 2,
	"CMP (<%b),Y", 2, 5,
	"CMP (<%b)", 2, 5,
	"CMP (<%b,S),Y", 2, 7,
	"PSH %b", 2, 6, // PEI
	"CMP <%b,X", 2, 4,
	"DEC <%b,X", 2, 6,
	"CMP [<%b],Y", 2, 6,
	// 0xD8
	"CLD", 1, 2,
	"CMP %w,Y", 3, 4,
	"PSH X", 1, 3, // PHX
	"HLT", 1, 3, // STP
	"JML (%w)", 3, 6,
	"CMP %w,X", 3, 4,
	"DEC %w,X", 3, 7,
	"CMP %L,X", 4, 5,
	// 0xE0
	"CPX %#i", 2, 2,
	"SBC (<%b,X)", 2, 6,
	"SEP %#b", 2, 3,
	"SBC <%b,s", 2, 4,
	"CPX <%b", 2, 3,
	"SBC <%b", 2, 3,
	"INC <%b", 2, 5,
	"SBC [<%b]", 2, 6,
	// 0xE8
	"INX", 1, 2,
	"SBC %#", 2, 2,
	"NOP", 1, 2,
	"SWA", 1, 3,
	"CPX %w", 3, 4,
	"SBC %w", 3, 4,
	"INC %w", 3, 6,
	"SBC %L", 4, 5,
	// 0xF0
	"BEQ @%b", 2, 2,
	"SBC (<%b),Y", 2, 5,
	"SBC (<%b)", 2, 5,
	"SBC (<%b,S),Y", 2, 7,
	"PSH %#w", 3, 5, // PEA
	"SBC <%b,X", 2, 4,
	"INC <%b,X", 2, 6,
	"SBC [<%b],Y", 2, 6,
	// 0xF8
	"SED", 1, 2,
	"SBC %w,Y", 3, 4,
	"PUL X", 1, 4, // PLX
	"XCE", 1, 2,
	"JSR (%w,X)", 3, 6,
	"SBC %w,X", 3, 4,
	"INC %w,X", 3, 7,
	"SBC %L,X", 4, 5
};


#endif //end DEBUGGER


