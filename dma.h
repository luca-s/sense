#ifndef DMA_H
#define DMA_H


/*                
  start a DMA transfer in the channel passed as argument.
  the possible values for channel are:
  	0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70
 (the channel is the offset used to access the DMA registers)
*/
extern void startDMA(int channel);

/*
  Call this function at the beginning of the frame.
  It activate the channel set in register 0x420C.
*/
extern void resethdma (void);

/*
  Call this function at the end of every scanline.
  For every channel activated by resethdma(), this function perform the
  hdma stuff.
*/
extern void dohdmaline (void);

extern byte hdma_inprogress;//tell us which hdma channels are enable

extern struct hdma{
	boolean continuous;
 	boolean isfirstline;
} HDMA[8];

#endif

