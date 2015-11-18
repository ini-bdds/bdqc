
#include "pophash.h"

/**
  * From "The Practice of Programming," Addison-Wesley, p.57
  * ISBN-13: 978-0201615869
  */
unsigned int pop_hash(char *str) {
   unsigned int h;
   unsigned char *p;

   h = 0;
   for (p = (unsigned char*)str; *p != '\0'; p++)
      h = MULTIPLIER * h + *p;
   return h; // or, h % ARRAY_SIZE;
}

