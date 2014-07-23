#include "utils.h"

void cpause(ticks cycles){
#if defined(XEON)
        cycles >>= 3;
        ticks i;
        for (i=0;i<cycles;i++) {
            _mm_pause();
        }
#elif defined(__arm__)
        __asm__ __volatile__ (
        "add.w %0, %0, #1       \n\t"
        "1:                     \n\t"
        "subs.w %0, %0, #1      \n\t"
        "bne 1b                 \n\t"
        : "+r" (cycles)
        );
#else
        ticks i;
        for (i=0;i<cycles;i++) {
            __asm__ __volatile__("nop");
        }
#endif
}

void cdelay(ticks cycles){
  ticks __ts_start = getticks();
  //ticks __ts_end = getticks() + (ticks) cycles;
  while (getticks() - __ts_start < (ticks) cycles); 
}
