#include <stdio.h>
#include "../libslock/include/utils.h"

main()
{

  ticks t1, t2;

  for (i = 0; i < 10; i ++) {
    t1 = getticks();
    t2 = getticks();
    printf("%d\n", t2 -t1);
  }
}

 
  
