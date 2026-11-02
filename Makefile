NAME = kernel.elf
ISO = kfs.iso

ASM = nasm
CC = gcc
LD = ld

ASMFLAGS = -f elf32
# -ffreestanding -fno-builtin -nostdlib -nodefaultlibs : aucune dépendance à la libc de l'host
CFLAGS = -m32 -ffreestanding -fno-builtin -fno-stack-protector \
         -fno-pie -fno-pic -nostdlib -nodefaultlibs -Wall -Wextra
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

BOOT_SRC = boot/boot.asm
KERNEL_SRC = kernel/kernel.c kernel/gdt.c kernel/printk.c kernel/stack.c

BOOT_OBJ = boot/boot.o
KERNEL_OBJ = $(KERNEL_SRC:.c=.o)

ISO_DIR = iso
ISO_KERNEL = $(ISO_DIR)/boot/kernel.elf
GRUB_CFG = $(ISO_DIR)/boot/grub/grub.cfg

.PHONY: all iso run clean fclean re

all: $(NAME)

$(BOOT_OBJ): $(BOOT_SRC)
	$(ASM) $(ASMFLAGS) $< -o $@

kernel/%.o: kernel/%.c kernel/kernel.h
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(BOOT_OBJ) $(KERNEL_OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJ) $(KERNEL_OBJ)

iso: $(ISO)

$(ISO): $(NAME) $(GRUB_CFG)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(NAME) $(ISO_KERNEL)
	grub-mkrescue -o $(ISO) $(ISO_DIR)

run: $(ISO)
	qemu-system-i386 -cdrom kfs.iso -m 32M -boot order=d,menu=off -nic none

clean:
	rm -f $(BOOT_OBJ) $(KERNEL_OBJ)

fclean: clean
	rm -f $(NAME) $(ISO) $(ISO_KERNEL)

re: fclean all
