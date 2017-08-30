/*Load rom, inizialize the emulator and call run() function (in cpu.h)*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "general.h"
#include "mmio.h"
#include "cpu.h"
#include "ppu.h"
#include "display.h"


struct settings snes_settings;

static struct headerdata lorom_header, hirom_header;//to store the header informations
struct headerdata * rom_header;

char* rom_name;//rom name

struct debug debugger;

void  resetDebugger(void)
{
 debugger.singleStep = TRUE;
 debugger.traceDMA = FALSE;
 debugger.traceRegs = FALSE;
 debugger.traceAudio = FALSE;
 debugger.traceExtra = TRUE;
 debugger.traceFPS  = FALSE;
}


/*the rom and the sram must already be loaded
  This function reset the snes and start emulator*/
void reset_snes(void)
{
 
#ifdef DEBUGGER
  resetDebugger();
#endif

 if (snes_settings.is_ntsc){
 	snes_settings.tv_hz = NTSC_TV_HZ;
	snes_settings.max_vcounter = SNES_NTSC_VCOUNTER_MAX;
 }
 else
 {
	snes_settings.tv_hz = PAL_TV_HZ;
	snes_settings.max_vcounter = SNES_PAL_VCOUNTER_MAX;
 }

 snes_settings.max_hcounter = SNES_HCOUNTER_MAX;

 resetmemory();					//reset the memory

 resetcpu();					//reset the cpu

 resetppu();					//reset ppu

 resetscheduler();				//reset the scheduler
 
 run();						//run the emulator

}

/*print a byte in binary form*/
static void printbyte(byte b){
	int i;
	for(i=7; i>=0; i--)
		if( b&(1<<i) ) putchar('1');
 		else putchar('0');
}


void show_rom_header(void){

	static const char *CoPro [16] = {
		" DSP1      ", " SuperFX ", " OBC1     ", "SA-1       ", "  S-DD1    ", " S-RTC     ", " CoPro#6   ",
		" CoPro#7 ", " CoPro#8 ", " CoPro#9 ", " CoPro#10 ", " CoPro#11 ", " CoPro#12 ",
		" CoPro#13 ", " CoPro#14 ", " CoPro-Custom "
	};
	static const char *Contents [3] = {
		" ROM        ", " ROM+RAM    ", " ROM+RAM+BAT"
	};

	printf("\nThese are the rom headers...\n");
	printf("\n                lorom header                  hirom header\n");
	printf("\nGame Title:     %-21s         %-21s",lorom_header.name,hirom_header.name);
	printf("\nRom makeup:     ");
	printbyte(lorom_header.makeup);
	((lorom_header.makeup & 0x0F) == 0x0 )?printf(" lorom/ "):printf("        ");
	((lorom_header.makeup & 0x10) == 0x10 )?printf("fast rom"):printf("slow rom");
	printf("      ");
	printbyte(hirom_header.makeup);
	((hirom_header.makeup & 0x0F) == 0x1 )?printf("  hirom/"):printf("        ");
	((hirom_header.makeup & 0x10) == 0x10 )?printf("fast rom"):printf("slow rom");
	printf("\nRom type:       ");
	printbyte(lorom_header.type);
	printf("%s",Contents [(lorom_header.type & 0xf) % 3]);
	((lorom_header.type & 0xf) >= 3) ?printf ("%s",CoPro [(lorom_header.type & 0xf0) >> 4]):
	printf("          ");
	printbyte(hirom_header.type);
	printf(" %s",Contents [(hirom_header.type & 0xf) % 3]);
	((hirom_header.type & 0xf) >= 3) ?printf ("  %s",CoPro [(hirom_header.type & 0xf0) >> 4]):printf("     ");
	printf("\nRom size:       ");
	printbyte(lorom_header.romsize);
	(lorom_header.romsize < 7 || lorom_header.romsize - 7 > 23)? printf(" Corrupt"):printf (" %dMbits ", (1<<(lorom_header.romsize - 7)));
	printf("             ");
	printbyte(hirom_header.romsize);
	(hirom_header.romsize < 7 || hirom_header.romsize - 7 > 23)? printf("  Corrupt"):printf ("  %dMbits", (1<<(hirom_header.romsize - 7)));
	printf("\nsram size:      ");
	printbyte(lorom_header.sramsize);
	(lorom_header.sramsize> 16)? printf(" Corrupt "):printf (" %dKbits ", 1<<(lorom_header.sramsize + 3));
	printf("            ");
	printbyte(hirom_header.sramsize);(hirom_header.sramsize> 16)? printf("  Corrupt"):printf ("  %dKbits", 1<<(hirom_header.sramsize + 3));
	printf("\ncountry:        ");
	printbyte(lorom_header.country);
	if(lorom_header.country==0) printf(" NTSC (JAP)");
	else if(lorom_header.country==1) printf(" NTSC (USA)");
	else printf(" PAL       ");
	printf("          ");
	printbyte(hirom_header.country);
	if(hirom_header.country==0) printf("  NTSC (JAP)");
	else if(hirom_header.country==1) printf("  NTSC (USA)");
	else printf("  PAL       ");
	printf("\nlicense:        ");
	printbyte(lorom_header.license);
	printf("                     ");
	printbyte(hirom_header.license);
	printf("\nversion:        ");
	printbyte(lorom_header.version);
	printf("                     ");
	printbyte(hirom_header.version);
	printf("\nchecksum        ");
	printbyte((byte)(lorom_header.checksum >> 8));
	printbyte(lorom_header.checksum);
	printf("             ");
	printbyte((byte)(hirom_header.checksum >> 8));
	printbyte(hirom_header.checksum);
	printf("\ninverse checksum");
	printbyte((byte)(lorom_header.complement >> 8));
	printbyte(lorom_header.complement);
	printf("             ");
	printbyte((byte)(hirom_header.complement >> 8));
	printbyte(hirom_header.complement);
	printf("\nNMI vector      %hx                   %hx", lorom_header.nmivector,hirom_header.nmivector);
	printf("\nReset vector    %hx                   %hx", lorom_header.resetvector,hirom_header.resetvector);


 	printf("\n\nThese are the value detected for this game: %s, %s, %s",snes_settings.is_hirom?"hirom":"lorom",snes_settings.is_fastrom?"fastrom":"slowrom",snes_settings.is_ntsc?"ntsc":"pal");

}


/*load the "rom_name" header informations*/
boolean loadromheaderdata (FILE* fp)
{
	word c[2];
	byte b;

		//try to autodetect the rom type(hirom or lorom)
	fseek (fp, 65472+21+rom_header->hadd, SEEK_SET);
	b = (fgetc (fp) & 0x0F);//getting rom makeup
	if (b == 1) {
		fseek (fp, 65472+28+rom_header->hadd, SEEK_SET);
		fread (c, sizeof (word), 2, fp);
	}
	if ( (b == 1) && ((c[0] ^ c[1]) == 0xFFFF) ) snes_settings.is_hirom = true;
	else snes_settings.is_hirom = false;

	//get the hirom header
		hirom_header.hirom = true;
		fseek (fp, 65530+hirom_header.hadd, SEEK_SET);
		fread (&hirom_header.nmivector, 2, 1, fp);
		fread (&hirom_header.resetvector, 2, 1, fp);
		fseek (fp, 65472+hirom_header.hadd, SEEK_SET);
		fread (&hirom_header.name, 21, 1, fp);
		hirom_header.name[21] = '\0';//I don't know if the title is a string
		hirom_header.makeup = fgetc (fp);//get makeup
		hirom_header.type = fgetc (fp); //get type
		hirom_header.romsize = fgetc (fp);
		hirom_header.sramsize = fgetc (fp);
		hirom_header.country = fgetc (fp);
		hirom_header.license = fgetc (fp);
		hirom_header.version = fgetc (fp);
		fread (&hirom_header.complement, sizeof (word), 1, fp);
		fread (&hirom_header.checksum, sizeof (word), 1, fp);
		if (hirom_header.resetvector == 0) hirom_header.resetvector = 0x8000;

	//get the lorom header
		lorom_header.hirom = false;
		fseek (fp, 32762+lorom_header.hadd, SEEK_SET);
		fread (&lorom_header.nmivector, 2, 1, fp);
		fread (&lorom_header.resetvector, 2, 1, fp);
		fseek (fp, 32704+lorom_header.hadd, SEEK_SET);
		fread (&lorom_header.name, 21, 1, fp);
		lorom_header.name[21] = '\0';//I don't know if the title is a string
		lorom_header.makeup = fgetc (fp);//get makeup
		lorom_header.type = fgetc (fp); //get type
		lorom_header.romsize = fgetc (fp);
		lorom_header.sramsize = fgetc (fp);
		lorom_header.country = fgetc (fp);
		lorom_header.license = fgetc (fp);
		lorom_header.version = fgetc (fp);
		fread (&lorom_header.complement, sizeof (word), 1, fp);
		fread (&lorom_header.checksum, sizeof (word), 1, fp);
		if (lorom_header.resetvector == 0) lorom_header.resetvector = 0x8000;

	if( snes_settings.is_hirom){
		if (hirom_header.makeup & 0x10) snes_settings.is_fastrom = true;
		else snes_settings.is_fastrom = false;
		if ( (hirom_header.country == 0) || (hirom_header.country == 1) ) snes_settings.is_ntsc = true;
		else snes_settings.is_ntsc = false;

	}else {
		if ( lorom_header.makeup & 0x10) snes_settings.is_fastrom = true;
		else snes_settings.is_fastrom = false;
		if ( (lorom_header.country == 0) || (lorom_header.country == 1) ) snes_settings.is_ntsc = true;
		else snes_settings.is_ntsc = false;

	}

	show_rom_header();//print the headers found

	return true;
}


boolean loadrom ()
{
	FILE *fp;
	int pos,x;

	if ( (fp = fopen (rom_name, "rb")) == NULL) return false;//open rom file

	printf ("...loading ROM: %s\n", rom_name);

	fseek (fp, rom_header->hadd, SEEK_SET);


	for ( pos = 0; !feof (fp); pos += 32768) {//storing the rom in memory

		x = fread (rom + pos, 1, 32768, fp);

		if( (x==0) && feof (fp)){
		 printf ("\n...loaded Megabits #%d\n", pos / (1024*128));
		 break;
		}

		if ( x < 32768 ) {//if an error occurred

			if( feof (fp) )
			{
			   printf("\nProbably rom corrupted but continue emulation\n");
			   break;
                                           }

			else printf("\nI/O error");

			return false;
		}

	}

	loadromheaderdata(fp);

	fclose (fp);

	return true;
}

boolean savesram(void){
 return false;
}

boolean loadsram(void){
 /*int x;
 for (x = 0; rom_name[x] != '\0'; x++)
	if (rom_name[x] == .) {

	}
*/
 return false;
}



void exit_snes(void)
{

 if( savesram() )				//save sram
 		printf("\n...sram saved");
 else printf("\nUnable to save sram");

 printf("\nThanks to Qwerty, Charles Bilyue', snes9x :) ... Bye Bye\n");
 
 exit(EXIT_SUCCESS);

}


/*sintax:  sense romname [d]
  so...
     argv[0] = "sense"
     argv[1] = "rom name"
    ?argv[2] = "s"?
    s = skip header
*/
int main(int argc, char* argv[]){

 int i=0;
 char options[21];

 if( argc < 2 || argc >3 ){
 		 printf("\nsintax:  sense romname [d]\n   option d = don't skip first 512 byte of rom header");
		 return EXIT_FAILURE;
 }

 if((argc == 3) && (argv[2][0]=='d'))    lorom_header.hadd = hirom_header.hadd = 0;
 else  lorom_header.hadd = hirom_header.hadd = 512; // assumes there is a 512-byte header
	 
 rom_header = &lorom_header;    //temporarely

 rom_name = argv[1] ;

 if( !loadrom()){				//load a rom
 		printf("\nUnable to load rom %s",rom_name);
	 	return EXIT_FAILURE;
 }

 printf("\n\n\nPlease digit the options and then press return:\n");
 printf("\nThe options are:\n  h = force hirom\n  l = force lorom\n  n =  ntsc system\n  p = pal system\n  f = force fast rom\n  s = slow rom\n  - = use autodetected values \n");
 scanf("%20s",options);

 if( options[0] != '-'){ //don't skip options

	for(i=0; options[i]!='\0'; i++ )//search for options...
		if(options[i]=='h') snes_settings.is_hirom = true;
		else if(options[i]=='l') snes_settings.is_hirom = false;
		else if(options[i]=='n') snes_settings.is_ntsc = true;
		else if(options[i]=='p') snes_settings.is_ntsc = false;
		else if(options[i]=='f') snes_settings.is_fastrom = true;
		else if(options[i]=='s') snes_settings.is_fastrom = false;
 }

 if( snes_settings.is_hirom == true ) rom_header = &hirom_header;
 else  rom_header = &lorom_header;

 if( initVideo() == -1)
	printf("\nProblem initializing video and input");

 if( loadsram())				//load a sram
 		printf("\n...sram loaded");
 else printf("\nUnable to load sram");

 printf("\n\n...starting snes\n\n\n");
 fflush(stdout);
 
 reset_snes();//reset and start snes

 /*never arrive here*/
 return EXIT_SUCCESS;
}


   
