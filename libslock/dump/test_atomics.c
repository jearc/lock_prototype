#include <stdint.h>
#include <stdio.h>

//test-and-set uint8_t
static inline uint8_t tas_uint8(volatile uint8_t *ptr) {
    uint8_t tmp1, atomic, old;

    __asm__ __volatile__ (
            "1:                             \n\t"
            "ldrexb %2, [%3]                \n\t"
            "ldrb %0, =1                    \n\t" /* set temp */
            "strexb %1, %0, [%3]            \n\t" /* set */
            "teq %1, #0                     \n\t" /* was atomic? */
            "bne 1b                         \n\t"
            : "=&r"(tmp1), "=&r"(atomic), "=&r"(old)          /* output */
            : "r"(ptr)                                      /* input */
            : "memory", "cc"                                /* clobbered */
            );

    return old;
}

//Swap uint8_t
static inline uint8_t swap_uint8(volatile uint8_t* ptr,  uint8_t x) {
    uint8_t atomic, ret;

    __asm__ __volatile__ (
            "1:                             \n\t"
            "ldrexb %1, [%2]                \n\t" /* ret = *ptr */
            "strexb %0, %3, [%2]            \n\t" /* *ptr = x */
            "teq %0, #0                     \n\t" /* was atomic? */
            "bne 1b                         \n\t"

            : "=&r"(atomic), "=&r"(ret)     /* output */
            : "r"(ptr), "r"(x)              /* input */
            : "memory", "cc"                /* clobbered */
            );

    return ret;
}

int main (int argc, char* argv []) {
    uint8_t a = atoi(argv[1]);

    //uint8_t old = tas_uint8(&a);
    uint8_t x = 3;
    uint8_t old = swap_uint8(&a, x);
    printf("a %d, old %d\n", a, old);

}
