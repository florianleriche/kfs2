#include "kernel.h"

/* Affiche un nombre non signé dans une base donnée.
 * width > 0 : complète avec des zéros à gauche (ex: %02x, %08x) */
static void print_unsigned(uint32_t n, uint32_t base, int width)
{
    const char *digits = "0123456789abcdef";
    char        buf[32];
    int         i = 0;

    if (n == 0)
        buf[i++] = '0';
    while (n > 0) {
        buf[i++] = digits[n % base];
        n /= base;
    }
    while (i < width)
        buf[i++] = '0';
    while (i > 0)
        putchar(buf[--i]); /* les chiffres ont été calculés à l'envers */
}

static void print_signed(int n)
{
    if (n < 0) {
        putchar('-');
        print_unsigned((uint32_t)0 - (uint32_t)n, 10, 0);
    } else
        print_unsigned((uint32_t)n, 10, 0);
}

/* Mini printf du kernel. Formats : %d %u %x %p %c %s %%
 * On peut préciser une largeur avec des zéros : %02x, %08x */
void printk(const char *fmt, ...)
{
    va_list args;
    int     width;

    va_start(args, fmt);
    while (*fmt) {
        if (*fmt != '%') {
            putchar(*fmt++);
            continue;
        }
        fmt++;                       /* saute le '%' */
        width = 0;
        if (*fmt == '0') {           /* ex: "%08x" -> width = 8 */
            fmt++;
            while (*fmt >= '0' && *fmt <= '9')
                width = width * 10 + (*fmt++ - '0');
        }
        if (*fmt == '\0')
            break;
        switch (*fmt) {
            case 'd': print_signed(va_arg(args, int));                    break;
            case 'u': print_unsigned(va_arg(args, uint32_t), 10, width);  break;
            case 'x': print_unsigned(va_arg(args, uint32_t), 16, width);  break;
            case 'p': print("0x");
                      print_unsigned(va_arg(args, uint32_t), 16, 8);      break;
            case 'c': putchar((char)va_arg(args, int));                   break;
            case 's': print(va_arg(args, const char *));                  break;
            case '%': putchar('%');                                       break;
            default : putchar('%'); putchar(*fmt);                        break;
        }
        fmt++;
    }
    va_end(args);
}
