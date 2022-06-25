CC = gcc
LD = ld

CFLAGS = -m32 -march=i386 -static \
	 -fno-builtin -fno-stack-protector -fno-omit-frame-pointer \
	 -Wall -Werror -O2 -I../lib
LDFLAGS = -m elf_i386

UCFILES = $(shell find ./ -name "*.c")
LCFILES = $(shell find ../lib -name "*.c")
UOBJS = $(UCFILES:.c=.o) $(LCFILES:.c=.o)


app_print.bin: $(UOBJS)
	$(LD) $(LDFLAGS) -e main -Ttext 0x00000000 -o app_print.elf $(UOBJS)
	objcopy -S -j .text -O binary app_print.elf app_print.elf 
#   objcopy -S -j .text -j .rodata -j .eh_frame -j .data -j .bss -O binary     app_print.elf app_print.bin
	@../utils/genApp.pl app_print.elf

clean:
	rm -rf $(UOBJS) app_print.elf
