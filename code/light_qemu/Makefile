CC=gcc
CFLAGS=-I.

light-qemu: qemu.c
	as -32 vm_os.s -o vm_os.o
	objcopy -O binary vm_os.o vm_os.bin
	$(CC) $(CFLAGS) -o light-qemu qemu.c 