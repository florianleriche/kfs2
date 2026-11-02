#include "kernel.h"

/* Symboles définis dans boot.asm : les deux bouts de la pile du kernel */
extern uint8_t stack_bottom[];
extern uint8_t stack_top[];

/* Affiche la pile du kernel de façon lisible :
 *   adresse : 16 octets en hexa | version ASCII |
 * On part de esp (le "haut" actuel de la pile) jusqu'à stack_top. */
void print_kernel_stack(void)
{
    uint32_t esp;
    uint32_t ebp;
    uint8_t *p;

    __asm__ volatile ("mov %%esp, %0" : "=r"(esp));
    __asm__ volatile ("mov %%ebp, %0" : "=r"(ebp));

    printk("=== Kernel stack ===\n");
    printk("stack_bottom = %p   stack_top = %p   taille  = %u octets\n",
           stack_bottom, stack_top, (uint32_t)(stack_top - stack_bottom));
    printk("esp          = %p   ebp       = %p   utilise = %u octets\n",
           esp, ebp, (uint32_t)stack_top - esp);

    p = (uint8_t *)esp;
    while (p < stack_top) {
        printk("%p: ", p);
        for (int i = 0; i < 16; i++) {
            if (p + i < stack_top)
                printk("%02x ", p[i]);
            else
                print("   ");
        }
        print(" |");
        for (int i = 0; i < 16 && p + i < stack_top; i++) {
            char c = (char)p[i];
            putchar((c >= 32 && c <= 126) ? c : '.');
        }
        print("|\n");
        p += 16;
    }
}
