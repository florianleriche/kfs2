#ifndef KERNEL_H
#define KERNEL_H

/* stdarg.h vient du compilateur (pas de la libc), donc autorisé en freestanding.
 * Il sert pour les fonctions à nombre d'arguments variable (printk). */
#include <stdarg.h>

/* Types kernel : pas de libc ici, donc on définit nous-mêmes les tailles fixes (i386) */
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned int   size_t;

/* kernel.c : écran VGA */
void clear_screen(void);
void putchar(char c);
void print(const char *s);

/* printk.c */
void printk(const char *fmt, ...);

/* gdt.c */
void gdt_init(void);

/* stack.c */
void print_kernel_stack(void);

#endif
