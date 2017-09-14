
#include "general.h"
#include "dma.h"
#include "mmio.h"
#include "cpu.h"

/*
  start a DMA transfer for the channel passed as argument.
  the possible values for channel are:
  	0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70
 (the channel is the offset used to access the DMA registers)
*/
void startDMA(int channel)
{
 dword dmacount;
 byte* regoffset[4];
 byte* reg;
 sdword auto_inc_dec;
 byte* tmp;
 union{
   word w;
   dword dw;
 }cpu;

 dmacount = *REG4305(channel);                 //get number of byte to transfer
 if( dmacount == 0) dmacount = 0x10000;
 
 cpu.dw = *REG4302(channel) & 0xFFFFFF;        //get the cpu memory address
 reg = registers + 0x0100 + *REG4301(channel);  //get the vram register address

 if (*REG4300(channel) & 0x10) // decrement
		auto_inc_dec = -1;
 else // Increment
		auto_inc_dec = 1;

#ifdef DEBUGGER
 if(  debugger.traceDMA )
 {			
 printf ("\nDMA[%x]: %s Mode: %d 0x%06X->0x21%02X Bytes: %d (%s) .....",
			channel, *REG4300(channel) & 0x80 ? "read" : "write",
			*REG4300(channel) & 7,  *REG4302(channel) & 0xFFFFFF,
			*REG4301(channel), dmacount,
			 *REG4300(channel) & 0x8? "fixed" :
		((*REG4300(channel) & 0x10) ? "dec" : "inc"));
 }
#endif

switch (*REG4300(channel) & 7) {//get the destination addresses for the current transfer mode

 				case 0: //1-address
				case 2: regoffset[0] = reg; regoffset[1] = reg;
					regoffset[2] = reg; regoffset[3] = reg;
					break;
					
				case 1: //2-address
				case 5: regoffset[0] = reg; regoffset[1] = reg+1;
					regoffset[2] = reg; regoffset[3] = reg+1;
					break;
					
				case 3: regoffset[0] = reg; regoffset[1] = reg;//2-address(Write Twice)
					regoffset[2] = reg+1; regoffset[3] = reg+1;
					break;
					
				case 4: regoffset[0] = reg; regoffset[1] = reg+1;//4-address
					regoffset[2] = reg+2; regoffset[3] = reg+3;
					break;
					
				default: DEBUG("\nInvalid DMA mode");
					break;
 }

 switch(*REG4300(channel) & 0x88){//check how the transfer must be done

	case 0x00://Cpu Memory -> Ppu      &&    Automatic Address Increment/decrement
	
	         tmp = SNESMEM(cpu.dw);
	         
	          do{
            
		     *(regoffset[0]) = *tmp;
		     trapregwrite (regoffset[0], false);
		     tmp  += auto_inc_dec;
		     if ( dmacount <= 1)
				break;
				
		     *(regoffset[1]) = *tmp;
		     trapregwrite (regoffset[1], false);
		     tmp  += auto_inc_dec;
		     if ( dmacount <= 2)
				break;
				
		     *(regoffset[2]) = *tmp;
		     trapregwrite (regoffset[2], false);
		     tmp  += auto_inc_dec;
		     if ( dmacount <= 3)
				break;
				
		     *(regoffset[3]) = *tmp;
		     trapregwrite (regoffset[3], false);
		     tmp  += auto_inc_dec;
                                
                                 dmacount -= 4;
                                
		  } while ( dmacount > 0 );//end of DMA transfer
		  
	          break;
	          
	case 0x08://Cpu Memory -> Ppu      &&    Fixed Address
	
	          tmp = SNESMEM(cpu.dw);
	          
	          do{
	          
	     	     *(regoffset[0]) = *tmp;
		     trapregwrite (regoffset[0], false);
		      if ( dmacount <= 1)
				break;
				
		     *(regoffset[1]) = *tmp;
		     trapregwrite (regoffset[1], false);
		      if ( dmacount <= 2)
				break;
				
		     *(regoffset[2]) = *tmp;
		     trapregwrite (regoffset[2], false);
		      if ( dmacount <= 3)
				break;
				
		     *(regoffset[3]) = *tmp;
		     trapregwrite (regoffset[3], false);
		                              
                                 dmacount -= 4;

		  } while ( dmacount > 0);//end of DMA transfer
		  
	          break;
	          
	case 0x80://Ppu -> Cpu Memory      &&    Automatic Address Increment/decrement
	
	          do{
	          
		 *SNESMEM(cpu.dw) = *trapregread ( regoffset[0] , false);
		 cpu.w = cpu.w + auto_inc_dec;
                        	if ( dmacount <= 1)
				break;
				
		*SNESMEM(cpu.dw) = *trapregread ( regoffset[1] , false);
		 cpu.w = cpu.w + auto_inc_dec;
             	               if ( dmacount <= 2)
				break;
             	              
		*SNESMEM(cpu.dw) = *trapregread ( regoffset[2] , false);
		 cpu.w = cpu.w + auto_inc_dec;
                            if ( dmacount <= 3)
				break;
             	              
             	              *SNESMEM(cpu.dw) = *trapregread ( regoffset[3] , false);
		 cpu.w = cpu.w + auto_inc_dec;
	              
                            dmacount -= 4;
	              
                       } while ( dmacount > 0 );//end of DMA transfer
	          break;
	          
	case 0x88://Ppu -> Cpu Memory      &&    Fixed Address
	          tmp = SNESMEM(cpu.dw);
	          do{

	              *tmp = *trapregread ( regoffset[0] , false);
		if ( dmacount <= 1)
				break;
				
             	              *tmp = *trapregread ( regoffset[1] , false);
             	               if ( dmacount <= 2)
				break;
             	              
             	              *tmp = *trapregread ( regoffset[2] , false);
             	              if ( dmacount <= 3)
				break;
             	              
             	              *tmp = *trapregread ( regoffset[3] , false);
	              
                            dmacount -= 4;
	              
	          } while ( dmacount > 0 );//end of DMA transfer
	          break;
 }

 *REG4302(channel) = (*REG4302(channel) & 0xFF000000) | cpu.dw;
 SCHEDULER.cycles -= (*REG4305(channel))? (*REG4305(channel)): 0x10000;
 *REG4305(channel) = 0;
  
#ifdef DEBUGGER
 if(  debugger.traceDMA )
 {			
 	printf ("...Terminated");
 }
#endif

  
}//end DMA function



byte hdma_inprogress = 0;//to remember the channels enabled at the beginning
                                //of the frame because register 0x420C may change

struct hdma HDMA[8];

/*
  Call this function at the beginning of the frame.
  It activates the channels set in register 0x420C for HDMA.
*/
void resethdma (void)
{
	int channel;

	hdma_inprogress = *REG420C;

	for (channel = 0x00; channel < 0x80; channel+=0x10) {
      
		if ( !(*REG420C & (1 << (channel>>4))) )//skip channel if not selected
			continue;

		*REG430A(channel) = 0;//line count = 0

		*REG4308(channel) = *(word*)REG4302(channel); // Pointer to table

		HDMA[channel>>4].isfirstline = TRUE;

	}

}

#define HBCOMBINE(a) ((*REG4302(channel) & 0xFF0000) | (a))

/*
  Call this function at the end of every scanline.
  For every channel activated by resethdma(), this function perform the
  hdma stuff.
*/
void dohdmaline (void)
{
	int channel, sh_chan;
	byte *dest;
	byte *source;
	int temp;
				/*for every channel*/
	for (channel = 0x00,sh_chan=0x00; channel < 0x80; channel+=0x10,sh_chan=channel>>4) {

		if ( !(hdma_inprogress & (1 << sh_chan ) ) )//skip channel if not enable
			continue;

		if ( *REG430A(channel) == 0) //if hdma counter expired set next values
		{
					/*get next count value*/
			temp = HBCOMBINE(*REG4308(channel));
			temp = *SNESMEM(temp);

			if (temp == 0x80)
			{
				HDMA[sh_chan].continuous = FALSE;
				*REG430A(channel) = 128;
			}
			else
			{
				HDMA[sh_chan].continuous = ( temp & 0x80 ) ? TRUE : FALSE;
				*REG430A(channel) = temp & 0x7f;
			}

			/*if end of table reached disable hdma channel*/
			if (temp == 0){
			 	hdma_inprogress &= ~(1 << sh_chan );
				continue;
			}

			HDMA[sh_chan].isfirstline = TRUE;
			(*REG4308(channel))++;

			if (*REG4300(channel) & 0x40) // Indirect
			{
			     	*(word*)REG4305(channel) = *(word*)SNESMEM(HBCOMBINE(*REG4308(channel)));
				*REG4308(channel)+=2;
			}

		}

		//check if hdma transfer doesn't need to be done
		if ( !HDMA[sh_chan].continuous && !HDMA[sh_chan].isfirstline)
		{
			(*REG430A(channel))--;//decrement scanline counter
			continue;
		}

			/*get transfer source address*/

		if ( *REG4300(channel) & 0x40 ) //Indirect HDMA
			temp = *(dword*)REG4305(channel) & 0xFFFFFF;

		else //regular HDMA
			temp =  HBCOMBINE (*REG4308(channel));

		source = SNESMEM ( temp );

			/*get transfer destination address*/

		dest = registers + 0x0100 + *REG4301(channel);


		switch (*REG4300(channel) & 0x7) {//check transfer mode and then start transfer

			case 0: // 1-address
				*dest = *source;     trapregwrite (dest, false);

				if ( *REG4300(channel) & 0x40 )
					*(word*)REG4305(channel) +=1;
				else
					*REG4308(channel) +=1;

				SCHEDULER.cycles -= 8;
				break;

			case 1: // 2-address
			case 5:
				*dest = *source;     trapregwrite (dest, false);

				dest++;
				*dest = *(source + 1); trapregwrite (dest, false);

				if ( *REG4300(channel) & 0x40 )
					*(word*)REG4305(channel) +=2;
				else
					*REG4308(channel) +=2;

				SCHEDULER.cycles -= 16;
				break;

			case 2: // 1-address Write Twice
			case 6:
				*dest = *source;     trapregwrite (dest, false);

				*dest = *(source + 1); trapregwrite (dest, false);

				if ( *REG4300(channel) & 0x40 )
					*(word*)REG4305(channel) +=2;
				else
					*REG4308(channel) +=2;

				SCHEDULER.cycles -= 16;
				break;

			case 3: // 2-address/write Twice
			case 7:
				*dest = *source;     trapregwrite (dest, false);

				*dest = *( source + 1 ); trapregwrite (dest, false);

				dest++;

				*dest = *( source + 2 ); trapregwrite (dest, false);

				*dest = *( source + 3 ); trapregwrite (dest, false);

				if ( *REG4300(channel) & 0x40 )
					*(word*)REG4305(channel) +=4;
				else
					*REG4308(channel) +=4;

				SCHEDULER.cycles -= 32;
				break;

			case 4: // 4-address
				*dest = *( source );     trapregwrite (dest, false);

				dest++;
				*dest = *( source + 1 ); trapregwrite (dest, false);

				dest++;
				*dest = *( source + 2 );  trapregwrite (dest, false);

				dest++;
				*dest = *( source + 3 ); trapregwrite (dest, false);

				if ( *REG4300(channel) & 0x40 )
					*(word*)REG4305(channel) +=4;
				else
					*REG4308(channel) +=4;

				SCHEDULER.cycles -= 32;
				break;

		}//end transfer


		HDMA[sh_chan].isfirstline = FALSE;

		(*REG430A(channel))--;//decrement scanline counter
	}
}
