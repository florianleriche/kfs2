#include "kernel.h"

/* Une entrée de GDT = 8 octets, dans l'ordre bizarre imposé par Intel.
 * __attribute__((packed)) interdit à gcc d'ajouter du padding entre les champs. */
struct gdt_entry {
    uint16_t limit_low;     /* limite, bits 0-15  */
    uint16_t base_low;      /* base,   bits 0-15  */
    uint8_t  base_middle;   /* base,   bits 16-23 */
    uint8_t  access;        /* droits : présent, ring, code/data, lecture/écriture */
    uint8_t  granularity;   /* 4 bits de flags (G, D/B) + limite bits 16-19 */
    uint8_t  base_high;     /* base,   bits 24-31 */
} __attribute__((packed));

/* Ce qu'on donne à l'instruction lgdt : la taille de la table - 1, et son adresse */
struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

#define GDT_ADDRESS 0x00000800  /* imposé par le sujet */
#define GDT_SIZE    7           /* null + 3 kernel + 3 user */

/* Octets d'accès (détail bit par bit dans Explications.md) */
#define ACCESS_KERNEL_CODE 0x9A /* 1001 1010 : présent, ring 0, code, lisible   */
#define ACCESS_KERNEL_DATA 0x92 /* 1001 0010 : présent, ring 0, data, écrivable */
#define ACCESS_USER_CODE   0xFA /* 1111 1010 : présent, ring 3, code, lisible   */
#define ACCESS_USER_DATA   0xF2 /* 1111 0010 : présent, ring 3, data, écrivable */

/* Flags : G=1 (limite en pages de 4 Ko), D/B=1 (segment 32 bits) */
#define FLAGS_32BIT_4K     0xC  /* 1100 */

/* Sélecteurs = index * 8 (utilisés dans boot.asm) :
 * 0x00 null  | 0x08 kcode | 0x10 kdata | 0x18 kstack
 * 0x20 ucode | 0x28 udata | 0x30 ustack */

static struct gdt_entry *gdt = (struct gdt_entry *)GDT_ADDRESS;
static struct gdt_ptr    gp;

extern void gdt_flush(struct gdt_ptr *ptr); /* boot.asm */

static void gdt_set_entry(int i, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t flags)
{
    gdt[i].limit_low   = limit & 0xFFFF;
    gdt[i].base_low    = base & 0xFFFF;
    gdt[i].base_middle = (base >> 16) & 0xFF;
    gdt[i].access      = access;
    gdt[i].granularity = ((limit >> 16) & 0x0F) | ((flags & 0x0F) << 4);
    gdt[i].base_high   = (base >> 24) & 0xFF;
}

void gdt_init(void)
{
    /* Modèle "flat" : chaque segment couvre toute la mémoire
     * (base 0, limite 4 Go). Seuls les droits changent d'une entrée à l'autre. */
    gdt_set_entry(0, 0, 0,       0,                  0);              /* null (obligatoire) */
    gdt_set_entry(1, 0, 0xFFFFF, ACCESS_KERNEL_CODE, FLAGS_32BIT_4K); /* kernel code  */
    gdt_set_entry(2, 0, 0xFFFFF, ACCESS_KERNEL_DATA, FLAGS_32BIT_4K); /* kernel data  */
    gdt_set_entry(3, 0, 0xFFFFF, ACCESS_KERNEL_DATA, FLAGS_32BIT_4K); /* kernel stack */
    gdt_set_entry(4, 0, 0xFFFFF, ACCESS_USER_CODE,   FLAGS_32BIT_4K); /* user code    */
    gdt_set_entry(5, 0, 0xFFFFF, ACCESS_USER_DATA,   FLAGS_32BIT_4K); /* user data    */
    gdt_set_entry(6, 0, 0xFFFFF, ACCESS_USER_DATA,   FLAGS_32BIT_4K); /* user stack   */

    gp.limit = sizeof(struct gdt_entry) * GDT_SIZE - 1; /* 7 * 8 - 1 = 55 = 0x37 */
    gp.base  = GDT_ADDRESS;

    gdt_flush(&gp); /* lgdt + rechargement des registres de segment */
}
