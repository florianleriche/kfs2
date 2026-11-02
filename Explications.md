# KFS_2 — Explications

KFS1 c'était « GRUB démarre mon kernel et j'affiche du texte ». KFS2 ajoute trois choses : **une GDT**, **un `printk`**, et **un outil qui affiche la pile**.

---

## 1. La GDT : c'est quoi et pourquoi

En mode protégé 32 bits, le CPU n'accède jamais à la mémoire avec une adresse brute. Chaque accès passe par un **segment**, et c'est la GDT (Global Descriptor Table) qui décrit ces segments. Chaque entrée dit :

- **base** : où commence le segment ;
- **limite** : sa taille ;
- **droits** : code ou données, ring 0 (kernel) ou ring 3 (user), lecture ou écriture.

GRUB nous fait déjà démarrer en mode protégé avec **sa propre** GDT. On ne sait pas où elle est en mémoire et on risque de l'écraser, donc on installe la nôtre.

### Modèle « flat »

Dans `kernel/gdt.c`, les 6 segments ont tous **base = 0 et limite = 4 Go**. Ils couvrent donc toute la mémoire et ne diffèrent que par leurs droits. C'est ce que font tous les OS modernes : la vraie protection de la mémoire viendra plus tard avec la pagination.

| Index | Sélecteur | Segment | Octet d'accès |
|---|---|---|---|
| 0 | 0x00 | null (obligatoire) | 0 |
| 1 | 0x08 | kernel code | 0x9A |
| 2 | 0x10 | kernel data | 0x92 |
| 3 | 0x18 | kernel stack | 0x92 |
| 4 | 0x20 | user code | 0xFA |
| 5 | 0x28 | user data | 0xF2 |
| 6 | 0x30 | user stack | 0xF2 |

### L'octet d'accès, bit par bit

```
0x9A = 1  00  1  1 0 1 0
       │  │   │  │ │ │ └ Accessed (le CPU le met à 1 tout seul)
       │  │   │  │ │ └── R/W : lisible (code) / écrivable (data)
       │  │   │  │ └──── direction/conforming : 0
       │  │   │  └────── Executable : 1 = code, 0 = data
       │  │   └───────── S : 1 = segment code/data (pas système)
       │  └───────────── DPL : niveau de privilège, 00 = ring 0
       └──────────────── Present : 1
```

- `0x92` : la même chose mais **non exécutable**, donc data, et écrivable.
- `0xFA` / `0xF2` : comme 0x9A / 0x92, mais avec **DPL = 11, donc ring 3** (user).

Dans le dump QEMU, on voit `93` au lieu de `92` : c'est le bit *Accessed* que le CPU a mis à 1 en chargeant le segment. C'est normal.

### Les flags : `0xC` = `1100`

- **G = 1** : la limite se compte en pages de 4 Ko, donc 0xFFFFF × 4 Ko = 4 Go.
- **D/B = 1** : segment 32 bits.

### Une entrée = 8 octets découpés bizarrement

Exemple avec `0x00cf9a000000ffff` (kernel code) :

```
00      c f      9a      00 0000    ffff
base    flags|   access  base       limite
24-31   lim16-19         0-23       0-15
```

Intel a gardé ce découpage pour rester compatible avec le 286. C'est pour ça que `struct gdt_entry` a cet ordre étrange, et qu'elle est en `__attribute__((packed))` pour que gcc n'ajoute pas de padding.

### Pourquoi 0x800 ?

C'est imposé par le sujet. `gdt` est un pointeur vers l'adresse `0x800` et on écrit les entrées directement là. Cette zone est libre : 0x000–0x4FF sert à la table d'interruptions du BIOS et à ses données, et la mémoire libre commence juste après.

---

## 2. Charger la GDT : `gdt_flush` (dans `boot/boot.asm`)

`gdt_init()` remplit une petite struct `gdt_ptr` : `limit = 7*8 - 1 = 0x37` et `base = 0x800`. Il passe ensuite la main à l'assembleur :

```asm
lgdt [eax]          ; "CPU, ta GDT est à 0x800, taille 0x37"
mov ax, 0x10        ; recharge DS/ES/FS/GS avec kernel data
...
mov ax, 0x18        ; SS = kernel stack
mov ss, ax
jmp 0x08:.flush     ; far jump pour recharger CS = kernel code
```

**Point clé :** `lgdt` dit seulement au CPU où se trouve la table. Les registres de segment gardent l'ancienne valeur tant qu'on ne les recharge pas. Et on **ne peut pas faire `mov cs, ax`**, d'où le far jump `jmp 0x08:...` : c'est la seule façon de changer CS.

**Sélecteur :** `0x08` = index 1 × 8. Les 3 bits du bas servent à autre chose : TI (GDT ou LDT) et RPL (ring demandé). Ici ils sont à 0.

---

## 3. La pile du kernel

Dans `boot/boot.asm` :

```asm
stack_bottom: resb 16384   ; 16 Ko réservés dans .bss
stack_top:
...
mov esp, stack_top
```

La pile **descend** : `esp` part de `stack_top`, chaque `push` ou `call` le fait reculer. Nouveauté KFS2 : `stack_bottom` et `stack_top` sont `global`, pour que le C puisse les lire.

---

## 4. `printk` (`kernel/printk.c`)

C'est un mini `printf`. Il utilise `<stdarg.h>`, qui est autorisé parce qu'il vient du compilateur et non de la libc. Il gère `%d %u %x %p %c %s %%` et la largeur avec des zéros (`%08x`).

`print_unsigned` calcule les chiffres à l'envers (`n % base`) dans un buffer, puis les affiche en sens inverse.

`putchar` (dans `kernel/kernel.c`) gère `\n`. Il n'y a **pas de scroll** (c'est un bonus de KFS1) : si l'écran est plein, l'écriture repart en haut de l'écran pour ne jamais écrire en dehors du buffer VGA. Le dump de la pile tient largement dans les 25 lignes. `memcpy` et `memset` sont codés dans le kernel : il n'y a pas de libc, et gcc peut générer des appels à ces fonctions de lui-même (par exemple pour initialiser un tableau local).

---

## 5. Afficher la pile (`kernel/stack.c`)

1. On lit `esp` et `ebp` avec de l'asm inline.
2. On affiche les bornes, la taille (16384) et l'espace utilisé (`stack_top - esp`).
3. On fait un hex dump de `esp` jusqu'à `stack_top`, 16 octets par ligne : adresse, octets en hexa, puis la version ASCII.

Exemple de ligne à l'écran :

```
0x00107fe0: 00 48 65 6c 6c 6f 20 4b 46 53 32 00 ef be ad de  |.Hello KFS2.....|
```

- `48 65 6c 6c 6f…` = `"Hello KFS2"`, la variable locale `msg` de `kmain`.
- `ef be ad de` = `0xDEADBEEF` à l'envers, parce que x86 est **little-endian**.
- Les valeurs du type `0x0010xxxx` dans la pile sont des **adresses de retour** : elles pointent dans le code, chargé à 1 Mo.

Ça prouve que l'outil lit vraiment la pile.

