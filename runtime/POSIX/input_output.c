#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

/*
 * Return the (stdio) flags for a given mode. Store the flags
 * to be passed to an open() syscall through *optr.
 * Return 0 on error.
 */
int convert_to_stdio_open_flags(const char *mode, int *optr) {
  int ret = 0, m = 0, o = 0;
  switch (*mode++) {
  case 'r': /* open for reading */
    ret = 1;
    m = O_RDONLY;
    o = 0;
    break;
  case 'w': /* open for writing */
    ret = 2;
    m = O_WRONLY;
    o = O_CREAT | O_TRUNC;
    break;
  case 'a': /* open for appending */
    ret = 2;
    m = O_WRONLY;
    o = O_CREAT | O_APPEND;
    break;
  default: /* illegal mode */
    errno = EINVAL;
    return 0;
  }
  /* [rwa]\+ or [rwa]b\+ means read and write */
  if (*mode == '+' || (*mode == 'b' && mode[1] == '+')) {
    ret = 3;
    m = O_RDWR;
  }
  *optr = m | o;
  return ret;
}

FILE *fopen(const char *file, const char *mode) {
  FILE *fp;
  int f, oflags = 0;
  if (convert_to_stdio_open_flags(mode, &oflags) == 0)
    return NULL;
  if ((fp = malloc(sizeof(FILE))) == NULL)
    return NULL;
  if (oflags & O_CREAT) {
    if ((f = open(file, oflags, S_IRWXU)) < 0)
      return NULL;
  } else {
    if ((f = open(file, oflags)) < 0)
      return NULL;
  }
#ifndef __FreeBSD__
  fp->_fileno = f;
  fp->_mode = oflags;
#else
  fp->_file = f;
  fp->_flags = oflags;
#endif
  return fp;
}

int get_file_descriptor(FILE *stream) {
  if (stream == stdin) {
    return 0;
  }
  if (stream == stdout) {
    return 1;
  }
  if (stream == stderr) {
    return 2;
  }
#ifndef __FreeBSD__
  return stream->_fileno;
#else
  return stream->_file;
#endif
}

int fgetc(FILE *stream) {
  if (stream == NULL) {
    return 0;
  }

  int fd = get_file_descriptor(stream);
  unsigned char buf;
  ssize_t read_byte = read(fd, &buf, 1);
  if (read_byte == 1) {
    return buf;
  } else {
    return EOF;
  }
}

#ifndef __FreeBSD__
int getc(FILE *stream) {
  if (stream == NULL) {
    return 0;
  }

  int fd = get_file_descriptor(stream);
  unsigned char buf;
  ssize_t read_byte = read(fd, &buf, 1);
  if (read_byte == 1) {
    return buf;
  } else {
    return EOF;
  }
}
#endif

size_t fread(void *buffer, size_t size, size_t count, FILE *stream) {
  if (stream == NULL) {
    return 0;
  }

  int fd = get_file_descriptor(stream);
  ssize_t read_byte = read(fd, buffer, size * count);
  if (read_byte == -1) {
    return 0;
  }
  return read_byte / size;
}

char *fgets(char *s, int n, FILE *stream) {
  if (stream == NULL) {
    return 0;
  }

  char *p = s;
  if (s == NULL || n <= 0) {
    return NULL;
  }

  int c = 0;
  while (--n > 0 && (c = getc(stream)) != EOF) {
    if ((*p++ = c) == '\n') {
      break;
    }
  }
  if (c == EOF && p == s) {
    return NULL;
  }
  *p = '\0';
  return s;
}

#ifndef __FreeBSD__
int getchar(void) { return getc(stdin); }
#endif

char *gets(char *s) {
  char *p = s;
  if (s == NULL || ferror(stdin) || feof(stdin)) {
    return NULL;
  }

  int c = 0;
  while ((c = getchar()) != EOF) {
    if (c == '\n') {
      break;
    }
    *p++ = c;
  }
  if (ferror(stdin) || (c == EOF && p == s)) {
    return NULL;
  }
  *p = '\0';
  return s;
}

int fputc(int c, FILE *stream) {
  if (stream == NULL) {
    return 0;
  }

  int fd = get_file_descriptor(stream);
  unsigned char symb = c;
  int write_byte = write(fd, &symb, 1);
  if (write_byte == 1) {
    return c;
  } else {
    return EOF;
  }
}

#ifndef __FreeBSD__
int putc(int c, FILE *stream) {
  if (stream == NULL) {
    return 0;
  }

  int fd = get_file_descriptor(stream);
  unsigned char symb = c;
  int write_byte = write(fd, &symb, 1);
  if (write_byte == 1) {
    return c;
  } else {
    return EOF;
  }
}
#endif

size_t fwrite(const void *buffer, size_t size, size_t count, FILE *stream) {
  if (stream == NULL) {
    return 0;
  }

  int fd = get_file_descriptor(stream);
  void *cop_buf = buffer;
  int write_byte = write(fd, cop_buf, size * count);
  if (write_byte == -1) {
    return 0;
  }
  return write_byte / size;
}

int fputs(const char *str, FILE *stream) {
  if (stream == NULL) {
    return 0;
  }

  if (str == NULL) {
    return EOF;
  }

  while (*str != '\0') {
    unsigned char c = *str;
    fputc(c, stream);
    str++;
  }

  return 1;
}

#ifndef __FreeBSD__
int putchar(int c) { return putc(c, stdout); }
#endif

int puts(const char *str) {
  int write_code = fputs(str, stdout);
  if (write_code == EOF) {
    return EOF;
  }
  write_code = putchar('\n');
  if (write_code == EOF) {
    return EOF;
  }
  return 1;
}
