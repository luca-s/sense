CC      = gcc
OPTIMIZATIONS = -g
CFLAGS  :=  -Wall  $(OPTIMIZATIONS)
ROOT	=  /usr/
SDLLIB  := -L$(ROOT)/lib -lSDL -lpthread
SDLINC  := -I$(ROOT)/include/SDL -D_REENTRANT
TARGET  = sense

$(TARGET): snes.o cpu.o mmio.o dma.o ppu.o display.o tileConverter.o
	$(CC) $(CFLAGS) -o $(TARGET) snes.o cpu.o mmio.o dma.o ppu.o display.o tileConverter.o $(SDLLIB)

snes.o: snes.c general.h cpu.h mmio.h ppu.h display.h
	$(CC) $(CFLAGS) -c snes.c

cpu.o: cpu.c cpuexec.c cpu.h general.h mmio.h dma.h ppu.h
	$(CC) $(CFLAGS) -c cpu.c

mmio.o: mmio.c mmio.h general.h dma.h ppu.h cpu.h display.h tileConverter.h
	$(CC) $(CFLAGS) -c mmio.c

dma.o: dma.c dma.h general.h mmio.h cpu.h
	$(CC) $(CFLAGS) -c dma.c

ppu.o: ppu.c ppu.h general.h display.h
	$(CC) $(CFLAGS) -c ppu.c

display.o: display.c display.h general.h ppu.h
	$(CC) $(CFLAGS) -c display.c $(SDLINC)

tileConverter.o: tileConverter.c tileConverter.h general.h display.h
	$(CC) $(CFLAGS) -c tileConverter.c

clean:
	@echo Cleaning up...
	@rm -f $(TARGET) *.o *~
	@echo Done.


exe: $(TARGET)
	@./$(TARGET)
