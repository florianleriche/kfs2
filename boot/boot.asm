BITS 32

; ---------------------------------------------------------------
; Header Multiboot 1 (inchangé depuis KFS1)
; ---------------------------------------------------------------

section .multiboot
align 4

MB_MAGIC    equ 0x1BADB002   ; valeur que GRUB cherche pour reconnaître un kernel multiboot
MB_FLAGS    equ 0            ; aucune option demandée à GRUB
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS) ; magic + flags + checksum doit faire 0

    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM


; ---------------------------------------------------------------
; Pile du kernel : 16 Ko réservés dans .bss
; stack_bottom / stack_top sont exportés (global) pour que le C
; puisse les lire et afficher la pile.
; ---------------------------------------------------------------

section .bss
align 16

global stack_bottom
global stack_top

stack_bottom:
    resb 16384
stack_top:


; ---------------------------------------------------------------
; Point d'entrée du kernel
; ---------------------------------------------------------------

section .text
global _start
extern kmain

_start:
    cli                  ; coupe les interruptions, rien n'est encore prêt pour les gérer
    mov esp, stack_top   ; la pile du host n'est pas garantie valide, on pose la nôtre
    call kmain           ; saute dans le code C

.hang:
    hlt                  ; si kmain revient, on arrête le CPU proprement
    jmp .hang


; ---------------------------------------------------------------
; gdt_flush(struct gdt_ptr *ptr)
; Charge notre GDT dans le CPU (lgdt) puis recharge tous les
; registres de segment pour qu'ils pointent sur nos nouvelles entrées.
; Sélecteurs = index * 8 :
;   0x08 = kernel code, 0x10 = kernel data, 0x18 = kernel stack
; ---------------------------------------------------------------

global gdt_flush

gdt_flush:
    mov eax, [esp + 4]   ; 1er argument : adresse de la struct gdt_ptr
    lgdt [eax]           ; dit au CPU : "ta GDT est là"

    mov ax, 0x10         ; sélecteur kernel data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ax, 0x18         ; sélecteur kernel stack
    mov ss, ax

    jmp 0x08:.flush      ; far jump : seule façon de recharger CS (kernel code)
.flush:
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
