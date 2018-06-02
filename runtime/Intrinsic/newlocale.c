#include "locale.h"
#include "klee/klee.h"

#ifdef __APPLE__
# include <string.h>
# include <xlocale.h>
#endif

locale_t newlocale(int category_mask, const char *locale,
                          locale_t base){
  return klee_newlocale(category_mask, locale, base);
}

#ifdef __APPLE__
int freelocale(locale_t locobj){
  klee_freelocale(locobj);
  return 0;
}
#else
void freelocale(locale_t locobj){
  klee_freelocale(locobj);
}
#endif
