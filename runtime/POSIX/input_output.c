#include <stdio.h>

int fgetc(FILE *stream) {
  int fd = fileno(stream);
  unsigned char buf;
  ssize_t read_byte = read(fd, &buf, 1);
  if (read_byte == 1) {
    return buf;
  } else {
    return EOF;
  }
}

int getc(FILE *stream) {
  int fd = fileno(stream);
  unsigned char buf;
  ssize_t read_byte = read(fd, &buf, 1);
  if (read_byte == 1) {
    return buf;
  } else {
    return EOF;
  }
}

size_t fread(void *buffer, size_t size, size_t count, FILE *stream) {
  int fd = fileno(stream);
  ssize_t read_byte = read(fd, buffer, size * count);
  if (read_byte == -1) {
    return 0;
  }
  return read_byte / size;
}

char *fgets(char *s, int n, FILE *stream) {
  char *p = s;
  if (s == NULL || n <= 0 || ferror(stream) || feof(stream)) {
    return NULL;
  }

  int c = 0;
  while (--n > 0 && (c = getc(stream)) != EOF) {
    if ((*p++ = c) == '\n') {
      break;
    }
  }
  if (ferror(stream) || (c == EOF && p == s)) {
    return NULL;
  }
  *p = '\0';
  return s;
}

int getchar(void) { return getc(stdin); }

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
  int fd = fileno(stream);
  unsigned char symb = c;
  int write_byte = write(fd, &symb, 1);
  if (write_byte == 1) {
    return c;
  } else {
    return EOF;
  }
}

int putc(int c, FILE *stream) {
  int fd = fileno(stream);
  unsigned char symb = c;
  int write_byte = write(fd, &symb, 1);
  if (write_byte == 1) {
    return c;
  } else {
    return EOF;
  }
}

size_t fwrite(const void *buffer, size_t size, size_t count, FILE *stream) {
  int fd = fileno(stream);
  void *cop_buf = buffer;
  int write_byte = write(fd, cop_buf, size * count);
  if (write == -1) {
    return 0;
  }
  return write_byte / size;
}

int fputs(const char *str, FILE *stream) {
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

int putchar(int c) { return putc(c, stdout); }

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
