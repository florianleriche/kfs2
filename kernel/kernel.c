#include "kernel.h"

#define VGA_ADDRESS 0xB8000 /* adresse fixe du buffer texte VGA */
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_COLOR   0x0F     /* blanc sur fond noir */

static uint16_t *const vga = (uint16_t *)VGA_ADDRESS;
static int cursor = 0; /* position du prochain caractère */

/* Lib de base : -nostdlib enlève la libc, et gcc peut appeler memset/memcpy tout seul */

size_t strlen(const char *s)
{
    size_t len = 0;

    while (s[len] != '\0')
        len++;
    return len;
}

void *memset(void *dst, int value, size_t n)
{
    uint8_t *bytes = (uint8_t *)dst;

    for (size_t i = 0; i < n; i++)
        bytes[i] = (uint8_t)value;
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *to = (uint8_t *)dst;
    const uint8_t *from = (const uint8_t *)src;

    for (size_t i = 0; i < n; i++)
        to[i] = from[i];
    return dst;
}

void clear_screen(void)
{
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = (VGA_COLOR << 8) | ' '; /* case VGA = (couleur << 8) | caractère */
    cursor = 0;
}

void putchar(char c)
{
    if (c == '\n')
        cursor = (cursor / VGA_WIDTH + 1) * VGA_WIDTH; /* début de la ligne suivante */
    else
        vga[cursor++] = (VGA_COLOR << 8) | (unsigned char)c;
    if (cursor >= VGA_WIDTH * VGA_HEIGHT)
        cursor = 0; /* pas de scroll (bonus KFS1) : on repart en haut pour ne pas sortir du buffer */
}

void print(const char *s)
{
    while (*s)
        putchar(*s++);
}

void kmain(void)
{
    /* Variables locales : elles vivent sur la pile, on les retrouvera dans le dump */
    char     msg[]  = "Hello KFS2";
    uint32_t magic  = 0xDEADBEEF;

    clear_screen();
    gdt_init();                      /* 1) construit la GDT à 0x800, la charge dans le CPU */

    printk("42 - KFS2\n");
    printk("GDT chargee a l'adresse %p (%d entrees, %d octets)\n", 0x800, 7, 7 * 8);
    printk("msg = \"%s\"  magic = 0x%x\n\n", msg, magic);

    print_kernel_stack();            /* 2) affiche la pile du kernel */

    while (1)
        __asm__ volatile ("hlt"); /* le kernel n'a rien d'autre à faire, on boucle */
}
