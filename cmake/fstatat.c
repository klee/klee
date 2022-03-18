#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>

int main(void) {
  struct stat buf;
  #pragma GCC diagnostic error "-Wnonnull"
  fstatat(0, NULL, &buf, 0);
}
