KFS_2 - GDT & Stack. Reprend le kernel de KFS_1 (boot GRUB, affichage VGA) et ajoute une GDT à 0x800, un `printk` et un outil qui affiche la pile du kernel.

Le détail des concepts (GDT, octets d'accès, pile, printk) est dans `Explications.md`.

Déroulé de l'exécution :

1. Build : `docker compose run --rm kfs bash -c "make re && make iso"`. `make re` compile `boot/boot.asm` (nasm) et les 4 fichiers de `kernel/` (gcc), puis `ld` les lie avec `linker.ld` pour produire `kernel.elf`. `make iso` fabrique `kfs.iso` avec `grub-mkrescue`.
2. GRUB charge `kernel.elf` à 1 Mo et saute sur `_start` (`boot/boot.asm`) : `cli`, `mov esp, stack_top` (pile de 16 Ko dans `.bss`, bornes `stack_bottom`/`stack_top` exportées en `global`), puis `call kmain`.
3. `kmain` (`kernel/kernel.c`) appelle `gdt_init()` (`kernel/gdt.c`) qui écrit 7 descripteurs de 8 octets directement à l'adresse `0x00000800` : null, kernel code (0x9A), kernel data (0x92), kernel stack (0x92), user code (0xFA), user data (0xF2), user stack (0xF2). Modèle flat : base 0, limite 0xFFFFF avec G=1 (4 Go), flags 0xC (32 bits, granularité 4 Ko).
4. `gdt_init` remplit `gdt_ptr` (limit = 7*8-1 = 0x37, base = 0x800) et appelle `gdt_flush` (`boot/boot.asm`) : `lgdt`, puis DS/ES/FS/GS = 0x10, SS = 0x18, et un far jump `jmp 0x08:.flush` pour recharger CS.
5. `printk` (`kernel/printk.c`) est un mini printf qui écrit dans le buffer VGA via `putchar` : `%d %u %x %p %c %s %%`, largeur avec zéros (`%02x`, `%08x`). `putchar` gère `\n` ; pas de scroll (bonus KFS1) : si l'écran est plein, l'écriture repart en haut.
6. `print_kernel_stack()` (`kernel/stack.c`) lit `esp` et `ebp` en asm inline puis affiche la mémoire de `esp` jusqu'à `stack_top`, 16 octets par ligne en hexa + ASCII. Les variables locales de `kmain` (`"Hello KFS2"`, `0xDEADBEEF`) sont visibles dans le dump.
7. `kmain` termine par `while (1) hlt`.

Sélecteurs : `0x08` kernel code, `0x10` kernel data, `0x18` kernel stack, `0x20` user code, `0x28` user data, `0x30` user stack.

Commandes utiles :

- build + iso : `docker compose run --rm kfs bash -c "make re && make iso"`
- lancer dans QEMU (texte) : `docker compose run --rm kfs bash -c "qemu-system-i386 -cdrom kfs.iso -m 32M -boot order=d,menu=off -nic none -display curses"`
- prouver que la GDT est à 0x800 (moniteur QEMU) : `docker compose run --rm kfs bash -c "qemu-system-i386 -cdrom kfs.iso -m 32M -boot order=d -nic none -display none -monitor stdio"` puis taper `info registers` (ligne `GDT= 00000800 00000037`, `CS =0008`, `DS =0010`, `SS =0018`) et `xp /7xg 0x800` (les 7 descripteurs), puis `quit`
- taille de l'image (doit être < 10 Mo) : `ls -la kfs.iso`
- nettoyer : `docker compose run --rm kfs bash -c "make fclean"`
