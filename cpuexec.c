/*this code implements the cpu opcode and the cpu main loop.
  Defining or not defining #A_16Bit, #XY_16Bit and #EMU_MODE you can generate
  different versions of the code. The aim of this is generate all the routines
  needed to emulate the different state of the processor.(e,m,x flags bits set
  or cleared)
  See cpu.c to better understand the code.

  This is the structure of the routine:

	for(;;){

		while (cycles > next_event)//until the end of the scanline
			      execute cpu opcode

	        hardwareHandling();

		}


*/
#undef XYTYPE
#undef ATYPE
#undef SIGNED_XYTYPE
#undef SIGNED_ATYPE
#undef XY_IS16BIT
#undef A_IS16BIT
#undef INDEX_X
#undef INDEX_Y
#undef ACCUM
#undef NV_SHIFT
#undef SIGN_BIT
#undef CARRY_BIT
#undef SIGN_BIT_XY
#undef SIGN_BIT_NUMBER
#undef BCDADDADJUST
#undef BCDSUBADJUST

#ifdef XY_16BIT
	#define XYTYPE word
	#define SIGNED_XYTYPE sword
	#define XY_IS16BIT 1
	#define SIGN_BIT_XY 0x8000
	#define INDEX_X reg.x.W
	#define INDEX_Y reg.y.W
#else
	#define XYTYPE byte
	#define SIGNED_XYTYPE sbyte
	#define XY_IS16BIT 0
	#define SIGN_BIT_XY 0x80
	#define INDEX_X reg.x.B.l
	#define INDEX_Y reg.y.B.l
#endif

#ifdef A_16BIT
	#define ATYPE word
	#define SIGNED_ATYPE sword
	#define A_IS16BIT 1
	#define NV_SHIFT 8
	#define SIGN_BIT 0x8000
	#define CARRY_BIT 0x10000
	#define SIGN_BIT_NUMBER 15
	#define BCDADDADJUST bcdaddadjust_word
	#define BCDSUBADJUST bcdsubadjust_word
	#define ACCUM reg.a.W
#else
	#define ATYPE byte
	#define SIGNED_ATYPE sbyte
	#define A_IS16BIT 0
	#define NV_SHIFT 0
	#define SIGN_BIT 0x80
	#define CARRY_BIT 0x100
	#define SIGN_BIT_NUMBER 7
	#define BCDADDADJUST bcdaddadjust_byte
	#define BCDSUBADJUST bcdsubadjust_byte
	#define ACCUM reg.a.B.l
#endif


#define ISWORDOP A_IS16BIT

#define cycles	SCHEDULER.cycles

	for(;;){//never exit. Except if chage m,x or e flags

	   ATYPE oldA; // used by macros
	   XYTYPE oldXY;
	   ATYPE *amem;

		/*execute a sequence of istructions untill the end of the scanline*/
	   while( cycles > SCHEDULER.next_event){
	                   
	   	/***********************this part need to be changed...too slow***********************/
		/***we can do the speed check only in the istructions that can change the PBR*********/
		   check_speed();
		/*********************************************************************************/

				/*fetch_opcode*/
#ifdef FETCHFAST
		opdata = *(dword*) SNESMEM(OPADDRESS);
#else
		mem = SNESMEM(OPADDRESS);
		opdata = *(dword*) TRAPREAD( mem );
#endif
		opcode = (byte)opdata;
	        	opdata >>= 8;
                                
#ifdef DEBUGGER
        	
	if( debugger.singleStep ){
		char command;
		int t;
	
          		printf("\n\n\n%02X:%04X   opcode:  %02X  %s  ",
			PBR,PC, opcode, opcodelist[opcode].format);
			
		for( t = 0;  t<opcodelist[opcode].bytes-1; t++ )	
		{
			printf("%lX  ",  (opdata >>  (8 * t)  )&0xFF);
		}

		for (t = 0; instrlist[t].mnemonic[0] != '\0'; t++) {
			if (instrlist[t].mnemonic[0] == opcodelist[opcode].format[0] &&
				instrlist[t].mnemonic[1] == opcodelist[opcode].format[1] &&
					instrlist[t].mnemonic[2] == opcodelist[opcode].format[2]) 
			{
				printf("         %s",instrlist[t].descr);
				break;
			}
		}
				
		printf("\n\nA:%04X    X:%04X    Y:%04X    D:%04X    DB:%02X         S:%04X      P:",
		           ACCUM_16BIT, INDEX_X_16BIT, INDEX_Y_16BIT,  D,  DBR, S);
		E?putchar('E'):putchar('e');
		check(NEGATIVE)?putchar('N'):putchar('n');
		check(OVERFLOW)?putchar('V'):putchar('v');
		check(MEMORY)?putchar('M'):putchar('m');
		check( INDEX)?putchar('X'):putchar('x');
		check(BCD)?putchar('D'):putchar('d');
		check(INT_DISABLE )?putchar('I'):putchar('i');
		check(ZERO )?putchar('Z'):putchar('z');
		check(CARRY )?putchar('C'):putchar('c');
		
		printf("\n\n NMI : %s , V-IRQ : %s, H-IRQ : %s", 
			PPU.NMIEnable ? "Enable":"Disable", PPU.VTimerEnabled ? "Enable":"Disable",
			PPU.HTimerEnabled ? "Enable":"Disable");

		printf("\n\n scanline = %d,   cycles =%d,  next event = %d, BGmode selected: %d",
			SCHEDULER.screen_scanline, cycles,SCHEDULER.next_event, PPU.BGMode);
		
		fflush(stdout);

		for(;;)
		{
			printf("\n\n >  "); 		
			fflush(stdout);
			command = getchar();
			
		              if( command== '\n' ) break;
		              
			switch( command ) 
			{
				case '?':
					printf("\nCommands list:");
					printf("\n  ? :  help");
					printf("\n  q :  exit emulator");
					printf("\n  m : show memory map");
					printf("\n  b : show background mode informations");
					printf("\n  g : run emulator without single step option");
					printf("\n  D : trace DMA");
					printf("\n  d : turn off DMA tracing");
					printf("\n  R : trace I/O registers read or write");
					printf("\n  r :  turn off I/O registrs read and write tracing");
					printf("\n  A : trace Audio registers read but not write");
					printf("\n  a : turn off Audio registers tracing");
					printf("\n  E : trace extra informations");
					printf("\n  e : turn off extra informations tracing");
                             			printf("\n  F : print frame per second");
                             			printf("\n  f : turn off frame per second information");
					break;
				case 'm':
					showMemoryMap();
					break;
                                                        case 'b':
                                 			printf("\n BGmode selected: %d\n", PPU.BGMode);
					if( PPU.BG[0].BGSize )
						printf("\n\n\n16 bit TIle size selected for background 0");
					if( PPU.BG[1].BGSize )
						printf("\n\n\n16 bit TIle size selected for background 1");
					if( PPU.BG[2].BGSize )
						printf("\n\n\n16 bit TIle size selected for background 2");
					if( PPU.BG[3].BGSize )
						printf("\n\n\n16 bit TIle size selected for background 3");
					if( PPU.BG3Priority)
						printf("\n\n\nbackground 3 priority bit setted");

					printf("\n Main screen:");
					if( !(PPU.reg212C  & 1))
						printf("\n BG0  disactivated");
					else printf("\n BG0  activated");
					if( !(PPU.reg212C  & 2))
						printf("\n BG1  disactivated");
					else printf("\n BG1  activated");
					if( !(PPU.reg212C  & 4))
						printf("\n BG2  disactivated");
					else printf("\n BG2  activated");
					if( !(PPU.reg212C  & 8))
						printf("\n BG3  disactivated");
					else printf("\n BG3  activated");
					printf("\n\n\n");
                                                        	break;

                                                        case 'g':
                             			debugger.singleStep = FALSE;
                             			break;
                             		case 'D':
                             			debugger.traceDMA = TRUE;
                             			printf("\n  trace DMA");
                             			break;
                             		case 'd':
                             			debugger.traceDMA = FALSE;
                             			printf("\n stop DMA tracing");
                             			break;
                             		case 'R':
                             			debugger.traceRegs = TRUE;
                             			printf("\n trace I/O registers read or write");
                             			break;
                             		case 'r':
                             			debugger.traceRegs = FALSE;
                             			printf("\n stop tracing I/O registers read or write");
                             			break;
                             		case 'A':
                             			debugger.traceAudio = TRUE;
                             			printf("\n trace Audio registers read");
                             			break;
                             		case 'a':
                             			debugger.traceAudio = FALSE;
                             			printf("\n stop tracing Audio registers read or write");
                             			break;
                             		case 'E':
                             			debugger.traceExtra = TRUE;
                             			printf("\n trace extra informations");
                             			break;
                             		case 'e':
                             			debugger.traceExtra = FALSE;
                             			printf("\n stop tracing extra informations");
                             			break;
                             		case 'F':
                             			debugger.traceFPS = TRUE;
                             			printf("\n print frame per second");
                             			break;
                             		case 'f':
                             			debugger.traceFPS = FALSE;
                             			printf("\n stop printing frame per second");
                             			break;
                             		case 'q':
                             		            	exit_snes();
                             		              break;
				
                             	 }
                             	 
 			while( command !='\n') command=getchar();
                              } 
	}  
	
#endif


		switch(opcode){  /*execute opcode*/

	/*define what to do at the end of each case statment (at the end of each opcode)*/
#define DOBREAK 	break

		case 0x00: //   "BRK", 2, 8, Has a signature word: %w two bytes ignored
			#ifndef EMU_MODE
				PC += 2; // Add 2 because one byte is the signature in native mode
				PUSHBYTE (PBR);
			#else
				PC++;//I'm not sure that in emulation mode is incremented only by one
			#endif
			PUSHWORD (PC);
			PUSHBYTE (getStatusRegister());
			set(INT_DISABLE);
			clear(BCD);
			#ifdef EMU_MODE
				OPADDRESS = *(word *)SNESMEM (0x00FFFE);//it should set PBR to 0
			#else   
				OPADDRESS = *(word *)SNESMEM (0x00FFE6);//it should set PBR to 0
			#endif
			cycles -= 8;
			DOBREAK;
		case 0x01: //   "ORA (<%b,X)", 2, 6,
			mem = DIR_INDEX_INDIR_X;
			ACCUM |= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x02: //   "CSP", 2, 8, Has one-byte signature %b
			#ifndef EMU_MODE
				PC += 2; // Add 2 because one byte is the signature in native mode
				PUSHBYTE (PBR);
			#else
				PC++;//I'm not sure that in emulation mode is incremented only by one
			#endif
			PUSHWORD (PC);
			PUSHBYTE (getStatusRegister());
			set(INT_DISABLE);
			clear(BCD);
			#ifdef EMU_MODE
				OPADDRESS = *(word *)SNESMEM (0x00FFF4);//it should set PBR to 0
			#else
				OPADDRESS = *(word *)SNESMEM (0x00FFE4);//it should set PBR to 0
			#endif
			cycles -= 8;
			DOBREAK;
		case 0x03: //   "ORA <%b,S", 2, 4,
			mem = S_REL;
			ACCUM |= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x04: //   "TSB <%b", 2, 5,
			mem = DIR;
			amem = (ATYPE*)TRAPREAD(mem);
			SETZERO (*amem & ACCUM);
			*amem |= ACCUM;
			TRAPWRITE((byte*)amem);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x05: //   "ORA <%b", 2, 3,
			mem = DIR;
			ACCUM |= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 3+A_IS16BIT; DOBREAK;
		case 0x06: //   "SHL <%b", 2, 5,
			mem = DIR;
			if (*(amem = (ATYPE*)TRAPREAD(mem)) & SIGN_BIT) set(CARRY);
			else clear(CARRY);
			*amem <<= 1;
			TRAPWRITE ((byte*)amem);
			SETZERO (*amem);
			SETNEGATIVE (*amem);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x07: //   "ORA [<%b]", 2, 6,
			mem = DIR_INDIR_LONG;
			ACCUM |= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x08: //   "PSH P", 1, 3, // PHP
			PUSHBYTE ( getStatusRegister() );
			PC += 1; cycles -= 3; DOBREAK;
		case 0x09: //   "ORA %#", 2, 2,
			ACCUM |= IMM_ACCUM;
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2+A_IS16BIT; cycles -= 2+A_IS16BIT; DOBREAK;
		case 0x0A: //   "SHL A", 1, 2,
			if (ACCUM & SIGN_BIT) set(CARRY);
			else clear(CARRY);
			ACCUM <<= 1;
			SETZERO (ACCUM);
			SETNEGATIVE (ACCUM);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x0B: //   "PSH D", 1, 4, // PHD Push Direct
			PUSHWORD (D);
			PC += 1; cycles -= 4; DOBREAK;
		case 0x0C: //   "TSB %w", 3, 6,
			mem = ABS;
			amem = (ATYPE*)TRAPREAD(mem);
			SETZERO (*amem & ACCUM);
			*amem |= ACCUM;
			TRAPWRITE ((byte*)amem);
			PC += 3; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x0D: //   "ORA %w", 3, 4,
			mem = ABS;
			ACCUM |= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x0E: //   "SHL %w", 3, 6,
			mem = ABS;
			if (*(amem = (ATYPE*)TRAPREAD(mem)) & SIGN_BIT) set(CARRY);
			else clear(CARRY);
			*amem <<= 1;
			TRAPWRITE ((byte*)amem);
			SETZERO (*amem);
			SETNEGATIVE (*amem);
			PC += 3; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x0F: //   "ORA %L", 4, 5,
			mem = ABS_LONG;
			ACCUM |= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x10: //   "BPL @%b", 2, 2,
			PC += 2; 
			BranchCheck2();
			if ( check(NEGATIVE) ) { // False
				cycles -= 2; DOBREAK;
			}
			PC += ((sbyte) opdata);
			cycles -= 3; DOBREAK;
		case 0x11: //   "ORA (<%b),Y", 2, 5,
			mem = DIR_INDIR_INDEX_Y;
			ACCUM |= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x12: //   "ORA (<%b)", 2, 5,
			mem = DIR_INDIR;
			ACCUM |= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x13: //   "ORA (<%b,S),Y", 2, 7,
			mem = S_REL_INDIR_INDEX_Y;
			ACCUM |= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 7+A_IS16BIT; DOBREAK;
		case 0x14: //   "TRB <%b", 2, 5,
			mem = DIR;
			amem = (ATYPE*)TRAPREAD(mem);
			SETZERO (*amem & ACCUM);
			*amem &= ~ACCUM;
			TRAPWRITE ((byte*)amem);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x15: //   "ORA <%b,X", 2, 4,
			mem = DIR_INDEX_X;
			ACCUM |= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x16: //   "SHL <%b,X",2, 6,
			mem = DIR_INDEX_X;
			if (*(amem = (ATYPE*)TRAPREAD(mem)) & SIGN_BIT) set(CARRY);
			else clear(CARRY);
			*amem <<= 1;
			TRAPWRITE ((byte*)amem);
			SETZERO (*amem);
			SETNEGATIVE (*amem);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x17: //   "ORA [<%b],Y", 2, 6,
			mem = DIR_INDIR_LONG_INDEX_Y;
			ACCUM |= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x18: //   "CLC", 1, 2,
			clear(CARRY);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x19: //   "ORA %w,Y", 3, 4,
			mem = ABS_INDEX_Y;
			ACCUM |= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x1A: //   "INC A", 1, 2,
			ACCUM++;
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x1B: //   "TAS", 1, 2,
			//(*((ATYPE*)&reg.A)) = ACCUM;
			S = ACCUM_16BIT;
			#ifdef EMU_MODE
				reg.s.B.h = 0x01;//to ensure the stack is located in page 1
			#endif
			// No effect on flags
			PC += 1; cycles -= 2; DOBREAK;
		case 0x1C: //   "TRB %w", 3, 6,
			mem = ABS;
			amem = (ATYPE*)TRAPREAD(mem);
			SETZERO (*amem & ACCUM);
			*amem &= ~ACCUM;
			TRAPWRITE ((byte*)amem);
			PC += 3; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x1D: //   "ORA %w,X", 3, 4,
			mem = ABS_INDEX_X;
			ACCUM |= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x1E: //   "SHL %w,X", 3, 7,
			mem = ABS_INDEX_X;
			if (*(amem = (ATYPE*)TRAPREAD(mem)) & SIGN_BIT) set(CARRY);
			else clear(CARRY);
			*amem <<= 1;
			TRAPWRITE ((byte*)amem);
			SETZERO (*amem);
			SETNEGATIVE (*amem);
			PC += 3; cycles -= 7+A_IS16BIT; DOBREAK;
		case 0x1F: //   "ORA %L,X", 4, 5,
			mem = ABS_LONG_INDEX_X;
			ACCUM |= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x20: //   "JSR %w", 3, 6,
			PUSHWORD ( (PC + 2) );// add 2 because rtl add 1
			PC = OPWORD;
			cycles -= 6; DOBREAK;
		case 0x21: //   "AND (<%b,X)", 2, 6,
			mem = DIR_INDEX_INDIR_X;
			ACCUM &= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x22: //   "JSL %L", 4, 8,
			PC+=3; // add 3 because rtl add 1
			PUSHBYTE ( PBR );
			PUSHWORD ( PC );
			OPADDRESS = OPLONG;
			cycles -= 8; DOBREAK;
		case 0x23: //   "AND <%b,s", 2, 4,
			mem = S_REL;
			ACCUM &= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x24: //   "BIT <%b", 2, 3,
			mem = DIR;
			if (*(amem = (ATYPE*)TRAPREAD(mem)) & ACCUM)
				clear(ZERO); else set(ZERO);
			(*amem & SIGN_BIT) ? set(NEGATIVE) : clear(NEGATIVE);
			(*amem & (SIGN_BIT >> 1) ) ? set(OVERFLOW) : clear(OVERFLOW);
			PC += 2; cycles -= 3+A_IS16BIT; DOBREAK;
		case 0x25: //   "AND <%b", 2, 3,
			mem = DIR;
			ACCUM &= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 3+A_IS16BIT; DOBREAK;
		case 0x26: //   "ROL <%b", 2, 5,
			mem = DIR;
			amem = (ATYPE*)TRAPREAD(mem);
			tmp = *amem & SIGN_BIT;
			*amem = (ATYPE)((*amem << 1) | check(CARRY));
			TRAPWRITE (mem);
			SETZERO (*amem);
			SETNEGATIVE (*amem);
			if (tmp) set(CARRY); else clear(CARRY);
			PC += 2; cycles -= 5; DOBREAK;
		case 0x27: //   "AND [<%b]", 2, 6,
			mem = DIR_INDIR_LONG;
			ACCUM &= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x28: //   "PUL P", 1, 4, // PLP
			PC += 1; cycles -= 4;
			   //pull status register
			#ifndef EMU_MODE
			   S++;
			#else
			   reg.s.B.l++;
			#endif
			 setStatusRegister( *(byte*)SNESMEM(S) );

			#ifdef EMU_MODE
				set(MEMORY);//to ensure that in emulation mode the memory
				set(INDEX); //and the index bit are not changed
				CHECK_FOR_IRQ();
				DOBREAK;
			#else
				#ifdef XY_16BIT
				if( check(INDEX) ){//index registers 8 bit
					INDEX_X_16BIT &= 0xFF; // clear high byte
					INDEX_Y_16BIT &= 0xFF;
				}
				#endif
				CHECK_FOR_IRQ(); 
				goto check_mx;
			#endif
		case 0x29: //   "AND %#", 2, 2,
			ACCUM &= IMM_ACCUM;
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2+A_IS16BIT; cycles -= 2+A_IS16BIT; DOBREAK;
		case 0x2A: //   "ROL A", 1, 2,
			tmp = ACCUM & SIGN_BIT;
			ACCUM = (ATYPE)((ACCUM << 1) | check(CARRY));
			SETZERO (ACCUM);
			SETNEGATIVE (ACCUM);
			if (tmp) set(CARRY); else clear(CARRY);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x2B: //   "PUL D", 1, 5, // PLD Pull Direct
			PULLWORD (D);
			SETZERO (D);
			if (D & 0x8000) set(NEGATIVE);
			else clear(NEGATIVE); // Don't use SETNEGATIVE
			PC += 1; cycles -= 5; DOBREAK;
		case 0x2C: //   "BIT %w", 3, 4,
			mem = ABS;
			if (*(amem = (ATYPE*)TRAPREAD(mem)) & ACCUM)
				clear(ZERO); else set(ZERO);
			(*amem & SIGN_BIT) ? set(NEGATIVE) : clear(NEGATIVE);
			(*amem & (SIGN_BIT >> 1) ) ? set(OVERFLOW) : clear(OVERFLOW);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x2D: //   "AND %w", 3, 4,
			mem = ABS;
			ACCUM &= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x2E: //   "ROL %w", 3, 6,
			mem = ABS;
			amem = (ATYPE*)TRAPREAD(mem);
			tmp = *amem & SIGN_BIT;
			*amem = (ATYPE)((*amem << 1) | check(CARRY));
			TRAPWRITE (mem);
			SETZERO (*amem);
			SETNEGATIVE (*amem);
			if (tmp) set(CARRY); else clear(CARRY);
			PC += 3; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x2F: //   "AND %L", 4, 5,
			mem = ABS_LONG;
			ACCUM &= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x30: //   "BMI @%b", 2, 2,
			PC += 2;
			BranchCheck2();
			if ( !check(NEGATIVE) ) { // False
				cycles -= 2; DOBREAK;
			}
			PC += ((sbyte) opdata);
			cycles -= 3; DOBREAK;
		case 0x31: //   "AND (<%b),Y", 2, 5,
			mem = DIR_INDIR_INDEX_Y;
			ACCUM &= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x32: //   "AND (<%b)", 2, 5,
			mem = DIR_INDIR;
			ACCUM &= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x33: //   "AND (<%b,S),Y", 2, 7,
			mem = S_REL_INDIR_INDEX_Y;
			ACCUM &= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 7+A_IS16BIT; DOBREAK;
		case 0x34: //   "BIT <%b,X", 2, 4,
			mem = DIR_INDEX_X;
			if (*(amem = (ATYPE*)TRAPREAD(mem)) & ACCUM)
				clear(ZERO); else set(ZERO);
			(*amem & SIGN_BIT) ? set(NEGATIVE) : clear(NEGATIVE);
			(*amem & (SIGN_BIT >> 1) ) ? set(OVERFLOW) : clear(OVERFLOW);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x35: //   "AND <%b,X", 2, 4,
			mem = DIR_INDEX_X;
			ACCUM &= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x36: //   "ROL <%b,X", 2, 6,
			mem = DIR_INDEX_X;
			amem = (ATYPE*)TRAPREAD(mem);
			tmp = *amem & SIGN_BIT;
			*amem = (ATYPE)((*amem << 1) | check(CARRY));
			TRAPWRITE (mem);
			SETZERO (*amem);
			SETNEGATIVE (*amem);
			if (tmp) set(CARRY); else clear(CARRY);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x37: //   "AND [<%b],Y", 2, 6,
			mem = DIR_INDIR_LONG_INDEX_Y;
			ACCUM &= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM);
			SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x38: //   "SEC", 1, 2,
			set(CARRY);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x39: //   "AND %w,Y", 3, 4,
			mem = ABS_INDEX_Y;
			ACCUM &= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x3A: //   "DEC A", 1, 2,
			ACCUM--;
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x3B: //   "TSA", 1, 2,
			ACCUM_16BIT = S;
			SETZERO (ACCUM); // Flags should change
			SETNEGATIVE (ACCUM);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x3C: //   "BIT %w,X", 3, 4,
			mem = ABS_INDEX_X;
			if (*(amem = (ATYPE*)TRAPREAD(mem)) & ACCUM)
				clear(ZERO); else set(ZERO);
			(*amem & SIGN_BIT) ? set(NEGATIVE) : clear(NEGATIVE);
			(*amem & (SIGN_BIT >> 1) ) ? set(OVERFLOW) : clear(OVERFLOW);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x3D: //   "AND %w,X", 3, 4,
			mem = ABS_INDEX_X;
			ACCUM &= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x3E: //   "ROL %w,X", 3, 7,
			mem = ABS_INDEX_X;
			amem = (ATYPE*)TRAPREAD(mem);
			tmp = *amem & SIGN_BIT;
			*amem = (ATYPE)((*amem << 1) | check(CARRY) );
			TRAPWRITE (mem);
			SETZERO (*amem);
			SETNEGATIVE (*amem);
   			if (tmp) set(CARRY); else clear(CARRY);
			PC += 3; cycles -= 7+A_IS16BIT; DOBREAK;
		case 0x3F: //   "AND %L,X", 4, 5,
			mem = ABS_LONG_INDEX_X;
			ACCUM &= *(ATYPE *)TRAPREAD(mem);
			SETZERO (ACCUM);
			SETNEGATIVE (ACCUM);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x40: //   "RTI", 1, 7,
			cycles -= 7;
			   //pull status register
			#ifndef EMU_MODE
			  S++;
			#else
			  reg.s.B.l++;
			#endif
			setStatusRegister( *(byte*)SNESMEM(S) );

			PULLWORD (PC);
			#ifndef EMU_MODE
				PULLBYTE (PBR);
				#ifdef XY_16BIT
				if( check(INDEX) ){//check if the x flag is set
					INDEX_X_16BIT &= 0xFF; // clear high byte
					INDEX_Y_16BIT &= 0xFF;
				}
				#endif
				CHECK_FOR_IRQ();
				goto check_mx;
			#else
				set(MEMORY);//to ensure that in emulation mode the memory
				set(INDEX); //and the index bit are not changed
				CHECK_FOR_IRQ();
				DOBREAK;
			#endif
		case 0x41: //   "XOR (<%b,X)", 2, 6,
			mem = DIR_INDEX_INDIR_X;
			ACCUM ^= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x42: //   "???", 2, 2,
			PC += 2; cycles -= 2; DOBREAK;
		case 0x43: //   "XOR <%b,S", 2, 4,
			mem = S_REL;
			ACCUM ^= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x44: //   "MVP XYA", 3, 7,
			cycles -= 7;
			DBR = OPBYTE;
			tmp = (opdata << 8) & 0x00FF0000;
			mem = SNESMEM (tmp + INDEX_X);
			mem2 = SNESMEM ( DBRSHIFTED + INDEX_Y);
			#ifdef BLOCKMOVEFAST
				*mem2 = *mem;
			#else
				#undef ISWORDOP
				#define ISWORDOP 0
				*mem2 = *TRAPREAD(mem);
				TRAPWRITE(mem2);
				#undef ISWORDOP
				#define ISWORDOP A_IS16BIT
			#endif
			INDEX_Y--;
			INDEX_X--;
			ACCUM--;
			if (ACCUM == (ATYPE) -1)
				PC += 3;
			DOBREAK;
		case 0x45: //   "XOR <%b", 2, 3,
			mem = DIR;
			ACCUM ^= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 3+A_IS16BIT; DOBREAK;
		case 0x46: //   "SHR <%b", 2, 5,
			mem = DIR;
			if (*(amem = (ATYPE*)TRAPREAD(mem)) & 1) set(CARRY);
			else clear(CARRY);
			*amem >>= 1;
			TRAPWRITE (mem);
			SETZERO (*amem);
			clear(NEGATIVE);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x47: //   "XOR [<%b]", 2, 6,
			mem = DIR_INDIR_LONG;
			ACCUM ^= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x48: //   "PSH A", 1, 3, // PHA
			#ifdef A_16BIT
				PUSHWORD (ACCUM);
			#else
				PUSHBYTE (ACCUM);
			#endif
			PC += 1; cycles -= 3+A_IS16BIT; DOBREAK;
		case 0x49: //   "XOR %#", 2, 2,
			ACCUM ^= IMM_ACCUM;
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2+A_IS16BIT; cycles -= 2+A_IS16BIT; DOBREAK;
		case 0x4A: //   "SHR A", 1, 2,
			if (ACCUM & 1) set(CARRY);
			else clear(CARRY);
			ACCUM >>= 1;
			SETZERO (ACCUM);
			clear(NEGATIVE);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x4B: //   "PSH PBR", 1, 3, // PHK
			PUSHBYTE (PBR);
			PC += 1; cycles -= 3; DOBREAK;
		case 0x4C: //   "JMP %w", 3, 3,
			PC = OPWORD;
			cycles -= 3; DOBREAK;
		case 0x4D: //   "XOR %w", 3, 4,
			mem = ABS;
			ACCUM ^= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM);
			SETNEGATIVE (ACCUM);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x4E: //   "SHR %w", 3, 6,
			mem = ABS;
			if (*(amem = (ATYPE*)TRAPREAD(mem)) & 1) set(CARRY);
			else clear(CARRY);
			*amem >>= 1;
			TRAPWRITE (mem);
			SETZERO (*amem);
			clear(NEGATIVE);
			PC += 3; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x4F: //   "XOR %L", 4, 5,
			mem = ABS_LONG;
			ACCUM ^= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x50: //   "BVC @%b", 2, 2,
			PC += 2;
			BranchCheck2();
			if ( check(OVERFLOW) ) { // False
				 cycles -= 2; DOBREAK;
			}
			PC += ((sbyte) opdata);
			cycles -= 3; DOBREAK;
		case 0x51: //   "XOR (<%b),Y", 2, 5,
			mem = DIR_INDIR_INDEX_Y;
			ACCUM ^= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x52: //   "XOR (<%b)", 2, 5,
			mem = DIR_INDIR;
			ACCUM ^= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x53: //   "XOR (<%b,S),Y", 2, 7,
			mem = S_REL_INDIR_INDEX_Y;
			ACCUM ^= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 7+A_IS16BIT; DOBREAK;
		case 0x54: //   "MVN %w", 3, 7,
			cycles -= 7;
			DBR = OPBYTE;
			tmp = (opdata << 8) & 0x00FF0000;
			mem = SNESMEM (tmp + INDEX_X);
			mem2 = SNESMEM ( DBRSHIFTED + INDEX_Y);
			#ifdef BLOCKMOVEFAST
				*mem2 = *mem;
			#else
				#undef ISWORDOP
				#define ISWORDOP 0
				*mem2 = *TRAPREAD(mem);
				TRAPWRITE(mem2);
				#undef ISWORDOP
				#define ISWORDOP A_IS16BIT
			#endif
			INDEX_Y++;
			INDEX_X++;
			ACCUM--;
			if (ACCUM == (ATYPE) -1)
				PC += 3;
			DOBREAK;
		case 0x55: //   "XOR <%b,X", 2, 4,
			mem = DIR_INDEX_X;
			ACCUM ^= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x56: //   "SHR <%b,X", 2, 6,
			mem = DIR_INDEX_X;
			if (*(amem = (ATYPE*)TRAPREAD(mem)) & 1) set(CARRY);
			else clear(CARRY);
			*amem >>= 1;
			TRAPWRITE (mem);
			SETZERO (*amem);
			clear(NEGATIVE);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x57: //   "XOR [<%b],Y", 2, 6,
			mem = DIR_INDIR_LONG_INDEX_Y;
			ACCUM ^= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x58: //   "CLI", 1, 2,
			clear(INT_DISABLE);
			PC += 1; cycles -= 2;
			CHECK_FOR_IRQ();
			DOBREAK;
		case 0x59: //   "XOR %w,Y", 3, 4,
			mem = ABS_INDEX_Y;
			ACCUM ^= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x5A: //   "PSH Y", 1, 3, // PHY
			#ifdef XY_16BIT
				PUSHWORD (INDEX_Y);
			#else
				PUSHBYTE (INDEX_Y);
			#endif
			PC += 1; cycles -= 3+XY_IS16BIT; DOBREAK;
		case 0x5B: //   "TAD", 1, 2,
			//(*((ATYPE*)&reg.A)) = ACCUM;
			D = ACCUM_16BIT;
			SETZERO (D); // Flags are changed
			SETNEGATIVE (D);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x5C: //   "JML %L", 4, 4,
			OPADDRESS = OPLONG;
			cycles -= 4; DOBREAK;
		case 0x5D: //   "XOR %w,X", 3, 4,
			mem = ABS_INDEX_X;
			ACCUM ^= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x5E: //   "SHR %w,X", 3, 7,
			mem = ABS_INDEX_X;
			if (*(amem = (ATYPE*)TRAPREAD(mem)) & 1) set(CARRY);
			else clear(CARRY);
			*amem >>= 1;
			TRAPWRITE (mem);
			SETZERO (*amem);
			clear(NEGATIVE);
			PC += 3; cycles -= 7+A_IS16BIT; DOBREAK;
		case 0x5F: //   "XOR %L,X", 4, 5,
			mem = ABS_LONG_INDEX_X;
			ACCUM ^= *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x60: //   "RTS", 1, 6,
			PULLWORD (PC);
			PC++;
			cycles -= 6; DOBREAK;
		case 0x61: //   "ADC (<%b,X)", 2, 6,
			SAVE(ACCUM);
			mem = DIR_INDEX_INDIR_X;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM + check(CARRY) + *amem;
			ADCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDADDADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x62: //   "PSH @%w", 3, 6, // PER
			PC += 3;
			PUSHWORD ( PC + OPWORD);
			cycles -= 6; DOBREAK;
		case 0x63: //   "ADC <%b,S", 2, 4,
			SAVE(ACCUM);
			mem = S_REL;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM + check(CARRY) + *amem;
			ADCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDADDADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x64: //   "CLR <%b", 2, 3,
			*(amem = (ATYPE*)DIR) = 0;
			TRAPWRITE ((byte*)amem);
			PC += 2; cycles -= 3+A_IS16BIT+A_IS16BIT; DOBREAK;
		case 0x65: //   "ADC <%b", 2, 3,
			SAVE(ACCUM);
			mem = DIR;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM + check(CARRY) + *amem;
			ADCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if ( check(BCD) ) BCDADDADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 3+A_IS16BIT; DOBREAK;
		case 0x66: //   "ROR <%b", 2, 5,
			mem = DIR;
			amem = (ATYPE*)TRAPREAD(mem);
			tmp = *amem & 1;
			*amem = (*amem >> 1) | ( check(CARRY) ? SIGN_BIT : 0);
			TRAPWRITE (mem);
			SETZERO (*amem);
			SETNEGATIVE (*amem);
			if (tmp) set(CARRY); else clear(CARRY);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x67: //   "ADC [<%b]", 2, 6,
			SAVE(ACCUM);
			mem = DIR_INDIR_LONG;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM + check(CARRY) + *amem;
			ADCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if ( check(BCD) ) BCDADDADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x68: //   "PUL A", 1, 4, // PLA
			#ifdef A_16BIT
				PULLWORD (ACCUM);
			#else
				PULLBYTE (ACCUM);
			#endif
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 1; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x69: /*   "ADC %#", 2, 2,*/
			SAVE(ACCUM);
			_tmpc = (dword)ACCUM + check(CARRY) + IMM_ACCUM;
			ADCSETCARRY(_tmpc);
			ACCUM =_tmpc;
			if ( check(BCD) ) BCDADDADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2+A_IS16BIT; cycles -= 2+A_IS16BIT; DOBREAK;
		case 0x6A: //   "ROR A", 1, 2,
			tmp = ACCUM & 1;
			ACCUM = (ACCUM >> 1) | ( check(CARRY) ? SIGN_BIT : 0);
			SETZERO (ACCUM);
			SETNEGATIVE (ACCUM);
			if (tmp) set(CARRY); else clear(CARRY);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x6B: //   "RTL", 1, 6,
			PULLWORD (PC);
			PULLBYTE (PBR);
			PC++; // If JSL %L only incs PC by 3, here there must be a final inc.
			cycles -= 6; DOBREAK;
		case 0x6C: //   "JMP (%w)", 3, 5,
			PC = *(word*)ABS_INDIR;
			cycles -= 5; DOBREAK;
		case 0x6D: /*   "ADC %w", 3, 4, */
			SAVE(ACCUM);
			mem = ABS;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM + check(CARRY) + *amem;
			ADCSETCARRY(_tmpc);
			ACCUM =_tmpc;
			if ( check(BCD) ) BCDADDADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x6E: //   "ROR %w", 3, 6,
			mem = ABS;
			amem = (ATYPE*)TRAPREAD(mem);
			tmp = *amem & 1;
			*amem = (*amem >> 1) | ( check(CARRY) ? SIGN_BIT : 0);
			TRAPWRITE (mem);
			SETZERO (*amem);
			SETNEGATIVE (*amem);
			if (tmp) set(CARRY); else clear(CARRY);
			PC += 3; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x6F: //   "ADC %L", 4, 5,
			SAVE(ACCUM);
			mem = ABS_LONG;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM + check(CARRY) + *amem;
			ADCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if ( check(BCD) ) BCDADDADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x70: //   "BVS @%b", 2, 2,
			PC += 2;
			BranchCheck2();
			if ( !check(OVERFLOW) ) { // False
				 cycles -= 2; DOBREAK;
			}
			PC +=  ((sbyte) opdata);
			cycles -= 3; DOBREAK;
		case 0x71: //   "ADC (<%b),Y", 2, 5,
			SAVE(ACCUM);
			mem = DIR_INDIR_INDEX_Y;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM + check(CARRY) + *amem;
			ADCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDADDADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x72: //   "ADC (<%b)", 2, 5,
			SAVE(ACCUM);
			mem = DIR_INDIR;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM + check(CARRY) + *amem;
			ADCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if ( check(BCD) ) BCDADDADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x73: //   "ADC (<%b,s),Y", 2, 7,
			SAVE(ACCUM);
			mem = S_REL_INDIR_INDEX_Y;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM + check(CARRY) + *amem;
			ADCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDADDADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 7+A_IS16BIT; DOBREAK;
		case 0x74: //   "CLR <%b,X", 2, 4,
			*(amem = (ATYPE*)DIR_INDEX_X) = 0;
			TRAPWRITE ((byte*)amem);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x75: //   "ADC <%b,X", 2, 4,
			SAVE(ACCUM);
			mem = DIR_INDEX_X;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM + check(CARRY) + *amem;
			ADCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDADDADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x76: //   "ROR <%b,X", 2, 6,
			mem = DIR_INDEX_X;
			amem = (ATYPE*)TRAPREAD(mem);
			tmp = *amem & 1;
			*amem = (*amem >> 1) | ( check(CARRY) ? SIGN_BIT : 0);
			TRAPWRITE (mem);
			SETZERO (*amem);
			SETNEGATIVE (*amem);
			if (tmp) set(CARRY); else clear(CARRY);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x77: //   "ADC [<%b],Y", 2, 6,
			SAVE(ACCUM);
			mem = DIR_INDIR_LONG_INDEX_Y;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM + check(CARRY) + *amem;
			ADCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDADDADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x78: //   "SEI", 1, 2,
			set(INT_DISABLE);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x79: //   "ADC %w,Y", 3, 4,
			SAVE(ACCUM);
			mem = ABS_INDEX_Y;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM + check(CARRY) + *amem;
			ADCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDADDADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x7A: //   "PUL Y", 1, 4, // PLY
			#ifdef XY_16BIT
				PULLWORD (INDEX_Y);
			#else
				PULLBYTE (INDEX_Y);
			#endif
			SETZERO (INDEX_Y); SETNEGATIVEXY (INDEX_Y);
			PC += 1; cycles -= 4+XY_IS16BIT; DOBREAK;
		case 0x7B: //   "TDA", 1, 2,
			ACCUM_16BIT = D;
			SETZERO (ACCUM); // Flags are changed
			SETNEGATIVE (ACCUM);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x7C: //   "JMP (%w,X)", 3, 6,
			PC = ABS_INDEX_INDIR_X_JMP;
			cycles -= 6; DOBREAK;
		case 0x7D: //   "ADC %w,X", 3,4,
			SAVE(ACCUM);
			mem = ABS_INDEX_X;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM + check(CARRY) + *amem;
			ADCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if ( check(BCD) ) BCDADDADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x7E: //   "ROR %w,X", 3,7,
			mem = ABS_INDEX_X;
			amem = (ATYPE*)TRAPREAD(mem);
			tmp = *amem & 1;
			*amem = (*amem >> 1) | ( check(CARRY) ? SIGN_BIT : 0);
			TRAPWRITE (mem);
			SETZERO (*amem);
			SETNEGATIVE (*amem);
			if (tmp) set(CARRY); else clear(CARRY);
			PC += 3; cycles -= 7+A_IS16BIT; DOBREAK;
		case 0x7F: //   "ADC %L,X",4,5,
			SAVE(ACCUM);
			mem = ABS_LONG_INDEX_X;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM + check(CARRY) + *amem;
			ADCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDADDADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x80: //   "BRA @%b", 2, 2,
			PC += 2 + ((sbyte) opdata);
			cycles -= 2; DOBREAK;
		case 0x81: //   "STA (<%b,X)", 2, 6,
			*(ATYPE*)(mem = DIR_INDEX_INDIR_X) = ACCUM;
			TRAPWRITE (mem);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x82: //   "BRL @%w", 3, 3,
			PC += 3 + (sword) OPWORD;
			cycles -= 3; DOBREAK;
		case 0x83: //   "STA <%b,S", 2, 4,
			*(ATYPE*)(mem = S_REL) = ACCUM;
			TRAPWRITE (mem);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x84: //   "STY <%b", 2, 3,
			*(XYTYPE*)(mem = DIR) = INDEX_Y;
				#undef ISWORDOP
				#define ISWORDOP XY_IS16BIT
			TRAPWRITE (mem);
				#undef ISWORDOP
				#define ISWORDOP A_IS16BIT
			PC += 2; cycles -= 3+XY_IS16BIT; DOBREAK;
		case 0x85: //   "STA <%b", 2, 3,
			*(ATYPE*)(mem = DIR) = ACCUM;
			TRAPWRITE (mem);
			PC += 2; cycles -= 3+A_IS16BIT; DOBREAK;
		case 0x86: //   "STX <%b", 2, 3,
			*(XYTYPE*)(mem = DIR) = INDEX_X;
				#undef ISWORDOP
				#define ISWORDOP XY_IS16BIT
			TRAPWRITE (mem);
				#undef ISWORDOP
				#define ISWORDOP A_IS16BIT
			PC += 2; cycles -= 3+XY_IS16BIT; DOBREAK;
		case 0x87: //   "STA [<%b]", 2, 6,
			*(ATYPE*)(mem = DIR_INDIR_LONG) = ACCUM;
			TRAPWRITE (mem);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x88: //   "DEY", 1, 2,
			INDEX_Y--;
			SETZERO (INDEX_Y); SETNEGATIVEXY (INDEX_Y);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x89: //   "BIT %#", 2, 2,
			if (ACCUM & IMM_ACCUM)
				clear(ZERO); else set(ZERO);
			PC += 2+A_IS16BIT; cycles -= 2+A_IS16BIT; DOBREAK;
		case 0x8A: //   "TXA", 1, 2,
			ACCUM = (ATYPE)INDEX_X_16BIT;
			SETZERO (ACCUM); // Flags are changed
			SETNEGATIVE (ACCUM);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x8B: //   "PSH DBR", 1, 3, // PHB
			PUSHBYTE (DBR);
			PC += 1; cycles -= 3; DOBREAK;
		case 0x8C: //   "STY %w", 3, 4,
			*(XYTYPE*)(mem = ABS) = INDEX_Y;
				#undef ISWORDOP
				#define ISWORDOP XY_IS16BIT
			TRAPWRITE (mem);
				#undef ISWORDOP
				#define ISWORDOP A_IS16BIT
			PC += 3; cycles -= 4+XY_IS16BIT; DOBREAK;
		case 0x8D: //   "STA %w", 3, 4,
			*(ATYPE*)(mem = ABS) = ACCUM;
			TRAPWRITE (mem);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x8E: //   "STX %w", 3, 4,
			*(XYTYPE*)(mem = ABS) = INDEX_X;
				#undef ISWORDOP
				#define ISWORDOP XY_IS16BIT
			TRAPWRITE (mem);
				#undef ISWORDOP
				#define ISWORDOP A_IS16BIT
			PC += 3; cycles -= 4+XY_IS16BIT; DOBREAK;
		case 0x8F: //   "STA %L", 4, 5,
			*(ATYPE*)(mem = ABS_LONG) = ACCUM;
			TRAPWRITE (mem);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x90: //   "BCC @%b", 2, 2,
			PC += 2; 
			BranchCheck2();
			if ( check(CARRY) ) { // False
				cycles -= 2; DOBREAK;
			}
			PC += ((sbyte) opdata);
			cycles -= 3; DOBREAK;
		case 0x91: //   "STA (<%b),Y", 2, 6,
			*(ATYPE*)(mem = DIR_INDIR_INDEX_Y) = ACCUM;
			TRAPWRITE (mem);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x92: //   "STA (<%b)", 2, 5,
			*(ATYPE*)(mem = DIR_INDIR) = ACCUM;
			TRAPWRITE (mem);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x93: //   "STA (<%b,S),Y", 2, 7,
			*(ATYPE*)(mem = S_REL_INDIR_INDEX_Y) = ACCUM;
			TRAPWRITE (mem);
			PC += 2; cycles -= 7+A_IS16BIT; DOBREAK;
		case 0x94: //   "STY <%b,X", 2, 4,
			*(XYTYPE*)(mem = DIR_INDEX_X) = INDEX_Y;
				#undef ISWORDOP
				#define ISWORDOP XY_IS16BIT
			TRAPWRITE (mem);
				#undef ISWORDOP
				#define ISWORDOP A_IS16BIT
			PC += 2; cycles -= 4+XY_IS16BIT; DOBREAK;
		case 0x95: //   "STA <%b,X", 2, 4,
			*(ATYPE*)(mem = DIR_INDEX_X) = ACCUM;
			TRAPWRITE (mem);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x96: //   "STX <%b,Y", 2, 4,
			*(XYTYPE*)(mem = DIR_INDEX_Y) = INDEX_X;
				#undef ISWORDOP
				#define ISWORDOP XY_IS16BIT
			TRAPWRITE (mem);
				#undef ISWORDOP
				#define ISWORDOP A_IS16BIT
			PC += 2; cycles -= 4+XY_IS16BIT; DOBREAK;
		case 0x97: //   "STA [<%b],Y", 2, 6,
			*(ATYPE*)(mem = DIR_INDIR_LONG_INDEX_Y) = ACCUM;
			TRAPWRITE (mem);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0x98: //   "TYA", 1, 2,
			//(*((XYTYPE*)&reg.Y)) = INDEX_Y;
			ACCUM = (ATYPE)INDEX_Y_16BIT;
			//ACCUM = (ATYPE)INDEX_Y;
			SETZERO (ACCUM); // Flags are changed
			SETNEGATIVE (ACCUM);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x99: //   "STA %w,Y", 3, 5,
			*(ATYPE*)(mem = ABS_INDEX_Y) = ACCUM;
			TRAPWRITE (mem);
			PC += 3; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x9A: //   "TXS", 1, 2,
			S = INDEX_X_16BIT;
			#ifdef EMU_MODE
				reg.s.B.h = 0x01;//to ensure the stack is located in page 1
			#endif
			// Has no effect on flags
			PC += 1; cycles -= 2; DOBREAK;
		case 0x9B: //   "TXY", 1, 2,
			INDEX_Y = INDEX_X;
			SETZERO (INDEX_Y); // Flags are changed
			SETNEGATIVEXY (INDEX_Y);
			PC += 1; cycles -= 2; DOBREAK;
		case 0x9C: //   "CLR %w", 3, 4,
			*(ATYPE*)(mem = ABS) = 0;
			TRAPWRITE (mem);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0x9D: //   "STA %w,X", 3, 5,
			*(ATYPE*)(mem = ABS_INDEX_X) = ACCUM;
			TRAPWRITE (mem);
			PC += 3; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x9E: //   "CLR %w,X", 3, 5,
			*(ATYPE*)(mem = ABS_INDEX_X) = 0;
			TRAPWRITE (mem);
			PC += 3; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0x9F: //   "STA %L,X", 4, 5,
			*(ATYPE*)(mem = ABS_LONG_INDEX_X) = ACCUM;
			TRAPWRITE (mem);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0xA0: //   "LDY %#i", 2, 2,
			INDEX_Y = IMM_INDEX;
			SETZERO (INDEX_Y); SETNEGATIVEXY (INDEX_Y);
			PC += 2+XY_IS16BIT; cycles -= 2+XY_IS16BIT; DOBREAK;
		case 0xA1: //   "LDA (<%b,X)", 2, 6,
			mem = DIR_INDEX_INDIR_X;
			ACCUM = *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0xA2: //   "LDX %#i", 2, 2,
			INDEX_X = IMM_INDEX;
			SETZERO (INDEX_X); SETNEGATIVEXY (INDEX_X);
			PC += 2+XY_IS16BIT; cycles -= 2+XY_IS16BIT; DOBREAK;
		case 0xA3: //   "LDA <%b,S", 2, 4,
			mem = S_REL;
			ACCUM = *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0xA4: //   "LDY <%b", 2, 3,
			mem = DIR;
			INDEX_Y = *(XYTYPE*)TRAPREAD(mem);
			SETZERO (INDEX_Y); SETNEGATIVEXY (INDEX_Y);
			PC += 2; cycles -= 3+XY_IS16BIT; DOBREAK;
		case 0xA5: //   "LDA <%b", 2, 3,
			mem = DIR;
			ACCUM = *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 3+A_IS16BIT; DOBREAK;
		case 0xA6: //   "LDX <%b", 2, 3,
			mem = DIR;
			INDEX_X = *(XYTYPE*)TRAPREAD(mem);
			SETZERO (INDEX_X); SETNEGATIVEXY (INDEX_X);
			PC += 2; cycles -= 3+XY_IS16BIT; DOBREAK;
		case 0xA7: //   "LDA [<%b]", 2, 6,
			mem = DIR_INDIR_LONG;
			ACCUM = *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0xA8: //   "TAY", 1, 2,
			//(*((ATYPE*)&reg.A)) = ACCUM;
			INDEX_Y = (XYTYPE)ACCUM_16BIT;
			//INDEX_Y = (XYTYPE)ACCUM;
			SETZERO (INDEX_Y); // Flags are changed
			SETNEGATIVEXY (INDEX_Y);
			PC += 1; cycles -= 2; DOBREAK;
		case 0xA9: //   "LDA %#", 2, 2,
			ACCUM = IMM_ACCUM;
			SETZERO (ACCUM);
			SETNEGATIVE (ACCUM);
			PC += 2+A_IS16BIT; cycles -= 2+A_IS16BIT; DOBREAK;
		case 0xAA: //   "TAX", 1, 2,
			//(*((ATYPE*)&reg.A)) = ACCUM;
			INDEX_X = (XYTYPE)ACCUM_16BIT;
			//INDEX_X = (XYTYPE)ACCUM;
			SETZERO (INDEX_X); // Flags are changed
			SETNEGATIVEXY (INDEX_X);
			PC += 1; cycles -= 2; DOBREAK;
		case 0xAB: //   "PUL DBR", 1, 4, // PLB
			PULLBYTE (DBR);
			SETZERO (DBR);
			if (DBR & 0x80) set(NEGATIVE);
			else clear(NEGATIVE);
			PC += 1; cycles -= 4; DOBREAK;
		case 0xAC: //   "LDY %w", 3, 4,
			mem = ABS;
			INDEX_Y = *(XYTYPE*)TRAPREAD(mem);
			SETZERO (INDEX_Y); SETNEGATIVEXY (INDEX_Y);
			PC += 3; cycles -= 4+XY_IS16BIT; DOBREAK;
		case 0xAD: //   "LDA %w", 3, 4,
			mem = ABS;
			ACCUM = *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0xAE: //   "LDX %w", 3, 4,
			mem = ABS;
			INDEX_X = *(XYTYPE*)TRAPREAD(mem);
			SETZERO (INDEX_X); SETNEGATIVEXY (INDEX_X);
			PC += 3; cycles -= 4+XY_IS16BIT; DOBREAK;
		case 0xAF: //   "LDA %L", 4, 5,
			mem = ABS_LONG;
			ACCUM = *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0xB0: //   "BCS @%b", 2, 2,
			PC += 2;
			BranchCheck2();
			if ( !check(CARRY) ) { // False
				cycles -= 2; DOBREAK;
			}
			PC +=((sbyte) opdata);
			cycles -= 3; DOBREAK;
		case 0xB1: //   "LDA (<%b),Y", 2, 5,
			mem = DIR_INDIR_INDEX_Y;
			ACCUM = *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0xB2: //   "LDA (<%b)", 2, 5,
			mem = DIR_INDIR;
			ACCUM = *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0xB3: //   "LDA (<%b,S),Y", 2, 7,
			mem = S_REL_INDIR_INDEX_Y;
			ACCUM = *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 7+A_IS16BIT; DOBREAK;
		case 0xB4: //   "LDY <%b,X", 2, 4,
			mem = DIR_INDEX_X;
			INDEX_Y = *(XYTYPE*)TRAPREAD(mem);
			SETZERO (INDEX_Y); SETNEGATIVEXY (INDEX_Y);
			PC += 2; cycles -= 4+XY_IS16BIT; DOBREAK;
		case 0xB5: //   "LDA <%b,X", 2, 4,
			mem = DIR_INDEX_X;
			ACCUM = *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0xB6: //   "LDX <%b,Y", 2, 4,
			mem = DIR_INDEX_Y;
			INDEX_X = *(XYTYPE*)TRAPREAD(mem);
			SETZERO (INDEX_X); SETNEGATIVEXY (INDEX_X);
			PC += 2; cycles -= 4+XY_IS16BIT; DOBREAK;
		case 0xB7: //   "LDA [<%b],Y", 2, 6,
			mem = DIR_INDIR_LONG_INDEX_Y;
			ACCUM = *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 2; cycles -= 6+A_IS16BIT;
			DOBREAK;
		case 0xB8: //   "CLV", 1, 2,
			clear(OVERFLOW);
			PC += 1; cycles -= 2; DOBREAK;
		case 0xB9: //   "LDA %w,Y", 3, 4,
			mem = ABS_INDEX_Y;
			ACCUM = *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0xBA: //   "TSX", 1, 2,
			INDEX_X = (XYTYPE) S;
			SETZERO (INDEX_X); // Flags are changed
			SETNEGATIVEXY (INDEX_X);
			PC += 1; cycles -= 2; DOBREAK;
		case 0xBB: //   "TYX", 1, 2,
			INDEX_X = INDEX_Y;
			SETZERO (INDEX_X); // Flags are changed
			SETNEGATIVE (INDEX_X);
			PC += 1; cycles -= 2; DOBREAK;
		case 0xBC: //   "LDY %w,X", 3, 4,
			mem = ABS_INDEX_X;
			INDEX_Y = *(XYTYPE*)TRAPREAD(mem);
			SETZERO (INDEX_Y); SETNEGATIVEXY (INDEX_Y);
			PC += 3; cycles -= 4+XY_IS16BIT; DOBREAK;
		case 0xBD: //   "LDA %w,X", 3, 4,
			mem = ABS_INDEX_X;
			ACCUM = *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0xBE: //   "LDX %w,Y", 3, 4,
			mem = ABS_INDEX_Y;
			INDEX_X = *(XYTYPE*)TRAPREAD(mem);
			SETZERO (INDEX_X); SETNEGATIVEXY (INDEX_X);
			PC += 3; cycles -= 4+XY_IS16BIT; DOBREAK;
		case 0xBF: //   "LDA %L,X",4, 5,
			mem = ABS_LONG_INDEX_X;
			ACCUM = *(ATYPE*)TRAPREAD(mem);
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0xC0: //   "CPY %#i", 2, 2,
			oldXY = INDEX_Y - IMM_INDEX;
			SETNEGATIVEXY (oldXY); SETZERO (oldXY);
			if (oldXY > INDEX_Y)
				clear(CARRY); else set(CARRY);
			PC += 2+XY_IS16BIT; cycles -= 2+XY_IS16BIT; DOBREAK;
		case 0xC1: //   "CMP (<%b,X)", 2, 6,
			mem = DIR_INDEX_INDIR_X;
			oldA = ACCUM - *(ATYPE*)TRAPREAD(mem);
			SETNEGATIVE (oldA);	SETZERO (oldA);
			if (oldA > ACCUM)
				clear(CARRY); else set(CARRY);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0xC2: //   "CLP %#b", 2, 3,
			PC += 2; cycles -= 3;
			#ifndef PFAST
				P &= ~IMM_BYTE;
			#else
				tmp = getStatusRegister();
				tmp &= ~IMM_BYTE;
				setStatusRegister(tmp);
			#endif
   			#ifdef EMU_MODE
				set(MEMORY);//to ensure that in emulation mode the memory
				set(INDEX); //and the index bit are not changed
				CHECK_FOR_IRQ();
				DOBREAK;
			#else
				#ifdef XY_16BIT
				if( check(INDEX) ){//index registers 8 bit
					INDEX_X_16BIT &= 0xFF; // clear high byte
					INDEX_Y_16BIT &= 0xFF;
				}
				#endif
				CHECK_FOR_IRQ();
				goto check_mx;
			#endif
		case 0xC3: //   "CMP <%b,S", 2, 4,
			mem = S_REL;
			oldA = ACCUM - *(ATYPE*)TRAPREAD(mem);
			SETNEGATIVE (oldA);	SETZERO (oldA);
			if (oldA > ACCUM)
				clear(CARRY); else set(CARRY);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0xC4: //   "CPY <%b", 2, 3,
			mem = DIR;
			oldXY = INDEX_Y - *(XYTYPE*)TRAPREAD(mem);
			SETNEGATIVEXY (oldXY); SETZERO (oldXY);
			if (oldXY > INDEX_Y)
				clear(CARRY); else set(CARRY);
			PC += 2; cycles -= 3+XY_IS16BIT; DOBREAK;
		case 0xC5: //   "CMP <%b", 2, 3,
			mem = DIR;
			oldA = ACCUM - *(ATYPE*)TRAPREAD(mem);
			SETNEGATIVE (oldA);	SETZERO (oldA);
			if (oldA > ACCUM)
				clear(CARRY); else set(CARRY);
			PC += 2; cycles -= 3+A_IS16BIT; DOBREAK;
		case 0xC6: //   "DEC <%b", 2, 5,
			mem = DIR;
			(*(amem = (ATYPE*)TRAPREAD(mem)))--;
			TRAPWRITE(mem);
			SETZERO (*amem); SETNEGATIVE (*amem);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0xC7: //   "CMP [<%b]", 2,6,
			mem = DIR_INDIR_LONG;
			oldA = ACCUM - *(ATYPE*)TRAPREAD(mem);
			SETNEGATIVE (oldA);	SETZERO (oldA);
			if (oldA > ACCUM)
				clear(CARRY); else set(CARRY);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0xC8: //   "INY", 1, 2,
			INDEX_Y++;
			SETZERO (INDEX_Y); SETNEGATIVEXY (INDEX_Y);
			PC += 1; cycles -= 2; DOBREAK;
		case 0xC9: //   "CMP %#", 2, 2,
			oldA = ACCUM - IMM_ACCUM;
			SETNEGATIVE (oldA);	SETZERO (oldA);
			if (oldA > ACCUM)
				clear(CARRY); else set(CARRY);
			PC += 2+A_IS16BIT; cycles -= 2+A_IS16BIT; DOBREAK;
		case 0xCA: //   "DEX", 1, 2,
			INDEX_X--;
			SETZERO (INDEX_X); SETNEGATIVEXY (INDEX_X);
			PC += 1; cycles -= 2; DOBREAK;
		case 0xCB: //   "WAI", 1, 3,
			if(CPU.irq)
			{
				PC++;
				cycles -=3;
			}
			else
			{
				CPU.WaitingForInterrupt = TRUE;
				cycles = SCHEDULER.next_event;
			}
			DOBREAK;
		case 0xCC: //   "CPY %w", 3, 4,
			mem = ABS;
			oldXY = INDEX_Y - *(XYTYPE*)TRAPREAD(mem);
			SETNEGATIVEXY (oldXY); SETZERO (oldXY);
			if (oldXY > INDEX_Y)
				clear(CARRY); else set(CARRY);
			PC += 3; cycles -= 4+XY_IS16BIT; DOBREAK;
		case 0xCD: //   "CMP %w", 3, 4,
			mem = ABS;
			oldA = ACCUM - *(ATYPE*)TRAPREAD(mem);
			SETNEGATIVE (oldA);	SETZERO (oldA);
			if (oldA > ACCUM)
				clear(CARRY); else set(CARRY);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0xCE: //   "DEC %w", 3, 6,
			mem = ABS;
			(*(amem = (ATYPE*)TRAPREAD(mem)))--;
			TRAPWRITE(mem);
			SETZERO (*amem); SETNEGATIVE (*amem);
			PC += 3; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0xCF: //   "CMP %L", 4, 5,
			mem = ABS_LONG;
			oldA = ACCUM - *(ATYPE*)TRAPREAD(mem);
			SETNEGATIVE (oldA);	SETZERO (oldA);
			if (oldA > ACCUM)
				clear(CARRY); else set(CARRY);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0xD0: //   "BNE @%b", 2, 2,
			PC += 2;
			BranchCheck2();
			if ( check(ZERO) ) { // False
				 cycles -= 2; DOBREAK;
			}
			PC += ((sbyte) opdata);
			cycles -= 3; DOBREAK;
		case 0xD1: //   "CMP (<%b),Y", 2, 5,
			mem = DIR_INDIR_INDEX_Y;
			oldA = ACCUM - *(ATYPE*)TRAPREAD(mem);
			SETNEGATIVE (oldA);	SETZERO (oldA);
			if (oldA > ACCUM)
				clear(CARRY); else set(CARRY);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0xD2: //   "CMP (<%b)", 2, 5,
			mem = DIR_INDIR;
			oldA = ACCUM - *(ATYPE*)TRAPREAD(mem);
			SETNEGATIVE (oldA);	SETZERO (oldA);
			if (oldA > ACCUM)
				clear(CARRY); else set(CARRY);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0xD3: //   "CMP (<%b,S),Y", 2, 7,
			mem = S_REL_INDIR_INDEX_Y;
			oldA = ACCUM - *(ATYPE*)TRAPREAD(mem);
			SETNEGATIVE (oldA);	SETZERO (oldA);
			if (oldA > ACCUM)
				clear(CARRY); else set(CARRY);
			PC += 2; cycles -= 7+A_IS16BIT; DOBREAK;
		case 0xD4: //   "PSH <%b", 2, 6, // PEI
			mem = DIR;
			PUSHWORD (*(word*)TRAPREAD(mem));
			PC += 2; cycles -= 6; DOBREAK;
		case 0xD5: //   "CMP <%b,X", 2, 4,
			mem = DIR_INDEX_X;
			oldA = ACCUM - *(ATYPE*)TRAPREAD(mem);
			SETNEGATIVE (oldA);	SETZERO (oldA);
			if (oldA > ACCUM)
				clear(CARRY); else set(CARRY);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0xD6: //   "DEC <%b,X", 2, 6,
			mem = DIR_INDEX_X;
			(*(amem = (ATYPE*)TRAPREAD(mem)))--;
			TRAPWRITE(mem);
			SETZERO (*amem); SETNEGATIVE (*amem);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0xD7: //   "CMP [<%b],Y", 2, 6,
			mem = DIR_INDIR_LONG_INDEX_Y;
			oldA = ACCUM - *(ATYPE*)TRAPREAD(mem);
			SETNEGATIVE (oldA);	SETZERO (oldA);
			if (oldA > ACCUM)
				clear(CARRY); else set(CARRY);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0xD8: //   "CLD", 1, 2,
			clear(BCD);
			PC += 1; cycles -= 2; DOBREAK;
		case 0xD9: //   "CMP %w,Y", 3, 4,
			mem = ABS_INDEX_Y;
			oldA = ACCUM - *(ATYPE*)TRAPREAD(mem);
			SETNEGATIVE (oldA);	SETZERO (oldA);
			if (oldA > ACCUM)
				clear(CARRY); else set(CARRY);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0xDA: //   "PSH X", 1, 3, // PHX
			#ifdef XY_16BIT
				PUSHWORD (INDEX_X);
			#else
				PUSHBYTE (INDEX_X);
			#endif
			PC += 1; cycles -= 3+XY_IS16BIT; DOBREAK;
		case 0xDB: //   "HLT", 1, 3, // STP
   			//PC += 1;
			cycles = -3;
			DEBUG("\n                    !!!snes halted");
			DOBREAK;
		case 0xDC: //   "JML (%w)", 3, 6,
			OPADDRESS = (*(dword*)ABS_INDIR) & 0xFFFFFF;
			cycles -= 6; DOBREAK;
		case 0xDD: //   "CMP %w,X", 3, 4,
			mem = ABS_INDEX_X;
			oldA = ACCUM - *(ATYPE*)TRAPREAD(mem);
			SETNEGATIVE (oldA);	SETZERO (oldA);
			if (oldA > ACCUM)
				clear(CARRY); else set(CARRY);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0xDE: //   "DEC %w,X", 3, 7,
			mem = ABS_INDEX_X;
			(*(amem = (ATYPE*)TRAPREAD(mem)))--;
			TRAPWRITE(mem);
			SETZERO (*amem); SETNEGATIVE (*amem);
			PC += 3; cycles -= 7+A_IS16BIT; DOBREAK;
		case 0xDF: //   "CMP %L,X", 4, 5,
			mem = ABS_LONG_INDEX_X;
			oldA = ACCUM - *(ATYPE*)TRAPREAD(mem);
			SETNEGATIVE (oldA);	SETZERO (oldA);
			if (oldA > ACCUM)
				clear(CARRY); else set(CARRY);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0xE0: //   "CPX %#i", 2, 2,
			oldXY = INDEX_X - IMM_INDEX;
			SETNEGATIVEXY (oldXY); SETZERO (oldXY);
			if (oldXY > INDEX_X)
				clear(CARRY); else set(CARRY);
			PC += 2+XY_IS16BIT; cycles -= 2+XY_IS16BIT; DOBREAK;
		case 0xE1: //   "SBC (<%b,X)", 2, 6,
			SAVE(ACCUM);
			mem = DIR_INDEX_INDIR_X;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM - ((check(CARRY) ? 0 : 1) + *amem);
			SBCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDSUBADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0xE2: //   "SEP %#b", 2, 3,
			PC += 2; cycles -= 3;
			#ifndef PFAST
				P |= IMM_BYTE;
			#else
				tmp = getStatusRegister();
				tmp |= IMM_BYTE;
				setStatusRegister(tmp);
			#endif
   			#ifdef EMU_MODE
				set(MEMORY);//to ensure that in emulation mode the memory
				set(INDEX); //and the index bit are not changed
				DOBREAK;
			#else
				#ifdef XY_16BIT
				if( check(INDEX) ){//index registers 8 bit
					INDEX_X_16BIT &= 0xFF; // clear high byte
					INDEX_Y_16BIT &= 0xFF;
				}
				#endif
				goto check_mx;
			#endif
		case 0xE3: //   "SBC <%b,S", 2, 4,
			SAVE(ACCUM);
			mem = S_REL;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM - ((check(CARRY) ? 0 : 1) + *amem);
			SBCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDSUBADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0xE4: //   "CPX <%b", 2, 3,
			mem = DIR;
			oldXY = INDEX_X - *(XYTYPE*)TRAPREAD(mem);
			SETNEGATIVEXY (oldXY); SETZERO (oldXY);
			if (oldXY > INDEX_X)
				clear(CARRY); else set(CARRY);
			PC += 2; cycles -= 3+XY_IS16BIT; DOBREAK;
		case 0xE5: //   "SBC <%b", 2, 3,
			SAVE(ACCUM);
			mem = DIR;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM - ((check(CARRY) ? 0 : 1) + *amem);
			SBCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDSUBADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 3+A_IS16BIT; DOBREAK;
		case 0xE6: //   "INC <%b", 2, 5,
			mem = DIR;
			(*(amem = (ATYPE*)TRAPREAD(mem)))++;
			TRAPWRITE(mem);
			SETZERO (*amem); SETNEGATIVE (*amem);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0xE7: //   "SBC [<%b]", 2, 6,
			SAVE(ACCUM);
			mem = DIR_INDIR_LONG;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM - ((check(CARRY) ? 0 : 1) + *amem);
			SBCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDSUBADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0xE8: //   "INX", 1, 2,
			INDEX_X++;
			SETZERO (INDEX_X); SETNEGATIVEXY (INDEX_X);
			PC += 1; cycles -= 2; DOBREAK;
		case 0xE9: //   "SBC %#", 2, 2,
			SAVE(ACCUM);
			_tmpc = (dword)ACCUM - ((check(CARRY) ? 0 : 1) + IMM_ACCUM);
			SBCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDSUBADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2+A_IS16BIT; cycles -= 2+A_IS16BIT; DOBREAK;
		case 0xEA: //   "NOP", 1, 2,
			PC += 1; cycles -= 2; DOBREAK;
		case 0xEB: //   "SWA", 1, 3,
			tmp = ACCUM_16BIT & 0xFF;
			ACCUM_16BIT >>= 8;
			ACCUM_16BIT |= tmp << 8;
			SETZERO (ACCUM); SETNEGATIVE (ACCUM);
			PC += 1; cycles -= 3; DOBREAK;
		case 0xEC: //   "CPX %w", 3, 4,
			mem = ABS;
			oldXY = INDEX_X - *(XYTYPE*)TRAPREAD(mem);
			SETNEGATIVEXY (oldXY); SETZERO (oldXY);
			if (oldXY > INDEX_X)
				clear(CARRY); else set(CARRY);
			PC += 3; cycles -= 4+XY_IS16BIT; DOBREAK;
		case 0xED: //   "SBC %w", 3, 4,
			SAVE(ACCUM);
			mem = ABS;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM - ((check(CARRY) ? 0 : 1) + *amem);
			SBCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDSUBADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0xEE: //   "INC %w", 3, 6,
			mem = ABS;
			(*(amem = (ATYPE*)TRAPREAD(mem)))++;
			TRAPWRITE(mem);
			SETZERO (*amem); SETNEGATIVE (*amem);
			PC += 3; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0xEF: //   "SBC %L", 4, 5,
			SAVE(ACCUM);
			mem = ABS_LONG;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM - ((check(CARRY) ? 0 : 1) + *amem);
			SBCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDSUBADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0xF0: //   "BEQ @%b", 2, 2,
			PC += 2;
			BranchCheck2();
			if ( !check(ZERO) ) { // False
				cycles -= 2; DOBREAK;
			}
			PC += ((sbyte) opdata);
			cycles -= 3; DOBREAK;
		case 0xF1: //   "SBC (<%b),Y", 2, 5,
			SAVE(ACCUM);
			mem = DIR_INDIR_INDEX_Y;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM - ((check(CARRY) ? 0 : 1) + *amem);
			SBCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDSUBADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0xF2: //   "SBC (<%b)", 2, 5,
			SAVE(ACCUM);
			mem = DIR_INDIR;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM - ((check(CARRY) ? 0 : 1) + *amem);
			SBCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDSUBADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 5+A_IS16BIT; DOBREAK;
		case 0xF3: //   "SBC (<%b,S),Y", 2, 7,
			SAVE(ACCUM);
			mem = S_REL_INDIR_INDEX_Y;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM - ((check(CARRY) ? 0 : 1) + *amem);
			SBCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDSUBADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 7+A_IS16BIT; DOBREAK;
		case 0xF4: //   "PSH %#w", 3, 5, // PEA
			PUSHWORD (OPWORD);
			PC += 3; cycles -= 5; DOBREAK;
		case 0xF5: //   "SBC <%b,X", 2, 4,
			SAVE(ACCUM);
			mem = DIR_INDEX_X;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM - ((check(CARRY) ? 0 : 1) + *amem);
			SBCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDSUBADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0xF6: //   "INC <%b,X", 2, 6,
			mem = DIR_INDEX_X;
			(*(amem = (ATYPE*)TRAPREAD(mem)))++;
			TRAPWRITE(mem);
			SETZERO (*amem); SETNEGATIVE (*amem);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0xF7: //   "SBC [<%b],Y", 2, 6,
			SAVE(ACCUM);
			mem = DIR_INDIR_LONG_INDEX_Y;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM - ((check(CARRY) ? 0 : 1) + *amem);
			SBCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDSUBADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 2; cycles -= 6+A_IS16BIT; DOBREAK;
		case 0xF8: //   "SED", 1, 2,
			set(BCD);
			PC += 1; cycles -= 2; DOBREAK;
		case 0xF9: //   "SBC %w,Y", 3, 4,
			SAVE(ACCUM);
			mem = ABS_INDEX_Y;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM - ((check(CARRY) ? 0 : 1) + *amem);
			SBCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDSUBADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0xFA: //   "PUL X", 1, 4, // PLX
			#ifdef XY_16BIT
				PULLWORD (INDEX_X);
			#else
				PULLBYTE (INDEX_X);
			#endif
			SETZERO (INDEX_X); SETNEGATIVEXY (INDEX_X);
			PC += 1; cycles -= 4+XY_IS16BIT; DOBREAK;
		case 0xFB: //   "XCE", 1, 2,
			PC += 1; cycles -= 2;
			tmp = check(CARRY);
			if ( E ) set(CARRY); else clear(CARRY);
			E = tmp;
			if ( E ) {
				set(MEMORY);
				set(INDEX);
				reg.s.B.h = 0x01;
				INDEX_X_16BIT &= 0xFF;
				INDEX_Y_16BIT &= 0xFF;
				//D = 0;//I'm not sure about this
			}
			goto check_mode;
		case 0xFC: //   "JSR (%w,X)", 3, 6,
			PUSHWORD ((PC + 2)); ;// add 2 because rtl add 1
			PC = ABS_INDEX_INDIR_X_JMP;
			cycles -= 6; DOBREAK;
		case 0xFD: //   "SBC %w,X", 3, 4,
			SAVE(ACCUM);
			mem = ABS_INDEX_X;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM - ((check(CARRY) ? 0 : 1) + *amem);
			SBCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDSUBADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 3; cycles -= 4+A_IS16BIT; DOBREAK;
		case 0xFE: //   "INC %w,X", 3, 7,
			mem = ABS_INDEX_X;
			(*(amem = (ATYPE*)TRAPREAD(mem)))++;
			TRAPWRITE(mem);
			SETZERO (*amem); SETNEGATIVE (*amem);
			PC += 3; cycles -= 7+A_IS16BIT; DOBREAK;
		case 0xFF: //   "SBC %L,X", 4, 5
			SAVE(ACCUM);
			mem = ABS_LONG_INDEX_X;
			amem = (ATYPE*)TRAPREAD(mem);
			_tmpc = (dword)ACCUM - ((check(CARRY) ? 0 : 1) + *amem);
			SBCSETCARRY(_tmpc);
			ACCUM = _tmpc;
			if (check(BCD)) BCDSUBADJUST (ACCUM);
			SETZERO(ACCUM);
			SETNEGATIVE(ACCUM); SETOVERFLOW(ACCUM, oldA);
			PC += 4; cycles -= 5+A_IS16BIT; DOBREAK;
		 }//end switch

            }//end scanline

	    hardwareHandling();

	}//end routine


#undef cycles		
#undef DOBREAK

