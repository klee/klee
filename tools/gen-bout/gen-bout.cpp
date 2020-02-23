//===-- gen-bout.cpp --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "klee/Internal/ADT/KTest.h"

#if defined(__FreeBSD__) || defined(__minix)
#define stat64 stat
#endif


#define MAX 64
static void push_obj(KTest *b, const char *name, unsigned total_bytes,
                     unsigned char *bytes) {
  KTestObject *o = &b->objects[b->numObjects++];
  assert(b->numObjects < MAX);

  o->name = strdup(name);
  o->numBytes = total_bytes;
  o->bytes = (unsigned char *)malloc(o->numBytes);

  memcpy(o->bytes, bytes, total_bytes);
}

static void push_range(KTest *b, const char *name, unsigned value) {
  KTestObject *o = &b->objects[b->numObjects++];
  assert(b->numObjects < MAX);

  o->name = strdup(name);
  o->numBytes = 4;
  o->bytes = (unsigned char *)malloc(o->numBytes);

  *(unsigned *)o->bytes = value;
}

void print_usage_and_exit(char *program_name) {
  fprintf(stderr,
    "%s: Tool for generating a ktest file from concrete input, e.g., for using a concrete crashing input as a ktest seed.\n"
    "Usage: %s <arguments>\n"
    "       <arguments> are the command-line arguments of the program, with the following treated as special:\n"
    "       --bout-file <filename>      - Specifying the output file name for the ktest file (default: file.bout).\n"
    "       --sym-stdin <filename>      - Specifying a file that is the content of stdin (only once).\n"
    "       --sym-stdout <filename>     - Specifying a file that is the content of stdout (only once).\n"
    "       --sym-file <filename>       - Specifying a file that is the content of a file named A provided for the program (only once).\n"
    "   Ex: %s -o -p -q file1 --sym-stdin file2 --sym-file file3 --sym-stdout file4\n",
    program_name, program_name, program_name);
  exit(1);
}

int main(int argc, char *argv[]) {
  unsigned i, argv_copy_idx;
  unsigned file_counter = 0;
  char *stdout_content_filename = NULL;
  char *stdin_content_filename = NULL;
  char *content_filenames_list[1024];
  char **argv_copy;
  char *bout_file = NULL;

  if (argc < 2)
    print_usage_and_exit(argv[0]);

  KTest b;
  b.symArgvs = 0;
  b.symArgvLen = 0;

  b.numObjects = 0;
  b.objects = (KTestObject *)malloc(MAX * sizeof *b.objects);

  if ((argv_copy = (char **)malloc(sizeof(char *) * argc * 2)) == NULL) {
    fprintf(stderr, "Could not allocate more memory\n");
    return 1;
  }

  argv_copy[0] = (char *)malloc(strlen(argv[0]) + 1);
  strcpy(argv_copy[0], argv[0]);
  argv_copy_idx = 1;

  for (i = 1; i < (unsigned)argc; i++) {
    if (strcmp(argv[i], "--sym-stdout") == 0 ||
        strcmp(argv[i], "-sym-stdout") == 0) {
      if (++i == (unsigned)argc || argv[i][0] == '-')
        print_usage_and_exit(argv[0]);

      if (stdout_content_filename)
        print_usage_and_exit(argv[0]);

      stdout_content_filename = argv[i];

    } else if (strcmp(argv[i], "--sym-stdin") == 0 ||
               strcmp(argv[i], "-sym-stdin") == 0) {
      if (++i == (unsigned)argc || argv[i][0] == '-')
        print_usage_and_exit(argv[0]);

      if (stdin_content_filename)
        print_usage_and_exit(argv[0]);

      stdin_content_filename = argv[i];
    } else if (strcmp(argv[i], "--sym-file") == 0 ||
               strcmp(argv[i], "-sym-file") == 0) {
      if (++i == (unsigned)argc || argv[i][0] == '-')
        print_usage_and_exit(argv[0]);

      content_filenames_list[file_counter++] = argv[i];
    } else if (strcmp(argv[i], "--bout-file") == 0 ||
               strcmp(argv[i], "-bout-file") == 0) {
      if (++i == (unsigned)argc)
        print_usage_and_exit(argv[0]);

      bout_file = argv[i];
    } else {
      long nbytes = strlen(argv[i]) + 1;
      static int total_args = 0;

      char arg[1024];
      snprintf(arg, sizeof(arg), "arg%02d", total_args++);
      push_obj(&b, (const char *)arg, nbytes, (unsigned char *)argv[i]);

      char *buf1 = (char *)malloc(1024);
      char *buf2 = (char *)malloc(1024);
      strcpy(buf1, "-sym-arg");
      snprintf(buf2, 1024, "%ld", nbytes - 1);
      argv_copy[argv_copy_idx++] = buf1;
      argv_copy[argv_copy_idx++] = buf2;
    }
  }

  if (file_counter > 0) {
    FILE *fp;
    struct stat64 file_stat;
    char *content_filename = content_filenames_list[file_counter - 1];

    if ((fp = fopen(content_filename, "r")) == NULL ||
        stat64(content_filename, &file_stat) < 0) {
      fprintf(stderr, "Failure opening %s\n", content_filename);
      print_usage_and_exit(argv[0]);
    }

    long nbytes = file_stat.st_size;
    char filename[7] = "A-data";
    char statname[12] = "A-data-stat";

    unsigned char *file_content, *fptr;
    if ((file_content = (unsigned char *)malloc(nbytes)) == NULL) {
      fputs("Memory allocation failure\n", stderr);
      exit(1);
    }

    int read_char;
    fptr = file_content;
    while ((read_char = fgetc(fp)) != EOF) {
      *fptr = (unsigned char)read_char;
      fptr++;
    }

    push_obj(&b, filename, nbytes, file_content);
    push_obj(&b, statname, sizeof(struct stat64), (unsigned char *)&file_stat);

    free(file_content);

    char *buf1 = (char *)malloc(1024);
    char *buf2 = (char *)malloc(1024);
    char *buf3 = (char *)malloc(1024);
    snprintf(buf1, 1024, "-sym-files");
    snprintf(buf2, 1024, "1");
    snprintf(buf3, 1024, "%ld", nbytes);
    argv_copy[argv_copy_idx++] = buf1;
    argv_copy[argv_copy_idx++] = buf2;
    argv_copy[argv_copy_idx++] = buf3;
  }

  if (stdin_content_filename) {
    FILE *fp;
    struct stat64 file_stat;
    char filename[6] = "stdin";
    char statname[11] = "stdin-stat";

    if ((fp = fopen(stdin_content_filename, "r")) == NULL ||
        stat64(stdin_content_filename, &file_stat) < 0) {
      fprintf(stderr, "Failure opening %s\n", stdin_content_filename);
      print_usage_and_exit(argv[0]);
    }

    unsigned char *file_content, *fptr;
    if ((file_content = (unsigned char *)malloc(file_stat.st_size)) == NULL) {
      fputs("Memory allocation failure\n", stderr);
      exit(1);
    }

    int read_char;
    fptr = file_content;
    while ((read_char = fgetc(fp)) != EOF) {
      *fptr = (unsigned char)read_char;
      fptr++;
    }

    push_obj(&b, filename, file_stat.st_size, file_content);
    push_obj(&b, statname, sizeof(struct stat64), (unsigned char *)&file_stat);

    free(file_content);

    char *buf1 = (char *)malloc(1024);
    char *buf2 = (char *)malloc(1024);
    snprintf(buf1, 1024, "-sym-stdin");
    snprintf(buf2, 1024, "%ld", file_stat.st_size);
    argv_copy[argv_copy_idx++] = buf1;
    argv_copy[argv_copy_idx++] = buf2;
  }

  if (stdout_content_filename) {
    FILE *fp;
    struct stat64 file_stat;
    unsigned char file_content[1024];
    char filename[7] = "stdout";
    char statname[12] = "stdout-stat";

    if ((fp = fopen(stdout_content_filename, "r")) == NULL ||
        stat64(stdout_content_filename, &file_stat) < 0) {
      fprintf(stderr, "Failure opening %s\n", stdout_content_filename);
      print_usage_and_exit(argv[0]);
    }

    int read_char;
    for (int i = 0; i < file_stat.st_size && i < 1024; ++i) {
      read_char = fgetc(fp);
      file_content[i] = (unsigned char)read_char;
    }

    for (int i = file_stat.st_size; i < 1024; ++i) {
      file_content[i] = 0;
    }

    file_stat.st_size = 1024;

    push_obj(&b, filename, 1024, file_content);
    push_obj(&b, statname, sizeof(struct stat64), (unsigned char *)&file_stat);

    char *buf = (char *)malloc(1024);
    snprintf(buf, 1024, "-sym-stdout");
    argv_copy[argv_copy_idx++] = buf;
  }

  argv_copy[argv_copy_idx] = 0;

  b.numArgs = argv_copy_idx;
  b.args = argv_copy;

  push_range(&b, "model_version", 1);

  if (!kTest_toFile(&b, bout_file ? bout_file : "file.bout"))
    assert(0);

  for (int i = 0; i < (int)b.numObjects; ++i) {
    free(b.objects[i].name);
    free(b.objects[i].bytes);
  }
  free(b.objects);

  for (int i = 0; i < (int)argv_copy_idx; ++i) {
    free(argv_copy[i]);
  }
  free(argv_copy);

  return 0;
}
