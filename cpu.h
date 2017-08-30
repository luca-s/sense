#ifndef CPU_H
#define CPU_H

/*must be called before running the emulator and after resetmemory()*/
extern void resetcpu(void);

/*must be called before running the emulator*/
extern void resetscheduler(void);

/*start the emulator execution. */
extern void run(void);

/*use it to activate an Horizontal IRQ at the correct point in a scanline*/
extern void setHIRQ (void);

extern void check_for_IRQ(void);

extern struct cpu{

 boolean irq;
 boolean WaitingForInterrupt;
 boolean branch_skip;//used for sound skip
 byte skip_method;
 
} CPU;         

extern struct scheduler{
        
 int cycles;//remaining cpu cycles before the end of the current scanline
 int screen_scanline; //the current scanline

 int next_event;//the cpu execution is stopped when cycles reach the next_event value. Then the hardware emulation is 
 int which_event;//performed (i.e. HDMA, NMI, etc. etc.). What exactly will be performed depend on which_event value

 int slowrom_cycles_per_scanline;//number of cpu cycles per scanline in case of slow rom speed
 int fastrom_cycles_per_scanline;//number of cpu cycles per scanline in case of fast rom speed

 int cycles_per_scanline;//the cycles per scanline currently used(equal to slowrom_cycles_per_scanline or fastrom_cycles_per_scanline)
 int fastrom_cycles;///the cycles per scanline currently setted for bank 80-FF(depends on 0x420D)

 /*h-blanking start*/
 int fastromHBlankStart;
 int slowromHBlankStart;

} SCHEDULER;

   
//events definition
#define END_SCANLINE	0
#define IRQ_REQUIRED	1

extern dword getPC(void);

#ifdef SOUND_SKIP

#define BranchCheck2()\
    if( CPU.branch_skip)\
    {\
         if(CPU.skip_method == 0)\
         {\
         	CPU.branch_skip = FALSE;\
         }\
         else if(CPU.skip_method == 1)\
         {\
	CPU.branch_skip = FALSE;\
	if( ((sbyte) opdata) > 0)\
	{\
                PC +=  ((sbyte) opdata);\
	  cycles -= 3; DOBREAK;\
              }\
         }\
         else if(CPU.skip_method == 2)\
         {\
	CPU.branch_skip = FALSE;\
	if( ((sbyte) opdata) < 0)\
	{\
	        cycles -= 2; DOBREAK;\
	}\
          }\
         else if(CPU.skip_method == 3)\
         {\
         	CPU.branch_skip = FALSE;\
	if( ((sbyte) opdata) < 0)\
	{\
	        cycles -= 2; DOBREAK;\
	}else\
	{\
                PC +=  ((sbyte) opdata);\
	  cycles -= 3; DOBREAK;\
              }\
         }\
   }
     
#else

#define BranchCheck2()

#endif
            
              

#endif
