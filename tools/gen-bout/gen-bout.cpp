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

  unsigned i;
  for (i = 0; i < total_bytes; i++)
    o->bytes[i] = bytes[i];
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
  fprintf(stderr, "Usage: %s <arguments>\n", program_name);
  fprintf(stderr, "       <arguments> are the command-line arguments of the "
                  "programs, with the following treated as special:\n");
  fprintf(stderr, "       --sym-stdin <filename>      - Specifying a file that "
                  "is the content of the stdin (only once).\n");
  fprintf(stderr, "       --sym-stdout <filename>     - Specifying a file that "
                  "is the content of the stdout (only once).\n");
  fprintf(stderr, "       --sym-file <filename>       - Specifying a file that "
                  "is the content of one of the files A, B, C, ... with\n");
  fprintf(stderr, "                                     the first occurrence "
                  "of --sym-file specifying the content of A and so on.\n");
  fprintf(stderr, "   Ex: %s -o -p -q file1 --sym-stdin file2 --sym-file file3 "
                  "--sym-file file4\n",
          program_name);
  exit(1);
}

int main(int argc, char *argv[]) {
  unsigned i;
  unsigned file_counter = 0;
  char *stdout_content_filename = NULL;
  char *stdin_content_filename = NULL;
  char *content_filenames_list[1024];

  if (argc < 2)
    print_usage_and_exit(argv[0]);

  KTest b;
  b.numArgs = argc;
  b.args = argv;
  b.symArgvs = 0;
  b.symArgvLen = 0;

  b.numObjects = 0;
  b.objects = (KTestObject *)malloc(MAX * sizeof *b.objects);

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
    } else {
      long nbytes = strlen(argv[i]) + 1;
      static int total_args = 0;

      char arg[1024];
      sprintf(arg, "arg%d", total_args++);
      push_obj(&b, (const char *)arg, nbytes, (unsigned char *)argv[i]);
    }
  }

  if (file_counter > 0) {
    for (i = 0; i < file_counter; ++i) {
      FILE *fp;
      struct stat64 file_stat;
      char *content_filename = content_filenames_list[i];

      if ((fp = fopen(content_filename, "r")) == NULL ||
          stat64(content_filename, &file_stat) < 0) {
        fprintf(stderr, "Failure opening %s\n", content_filename);
        print_usage_and_exit(argv[0]);
      }

      fseek(fp, 0L, SEEK_END);
      long nbytes = ftell(fp);
      fseek(fp, 0L, SEEK_SET);

      char filename[2] = "A";
      char statname[7] = "A-stat";

      filename[0] = statname[0] = 65 + i;

      unsigned char *file_content, *fptr;
      if ((file_content = (unsigned char *)malloc(nbytes)) == NULL) {
        fprintf(stderr, "Memory allocation failure\n");
        exit(1);
      }

      int read_char;
      fptr = file_content;
      while ((read_char = fgetc(fp)) != EOF) {
        *fptr = (unsigned char)read_char;
        fptr++;
      }

      push_obj(&b, filename, nbytes, file_content);
      push_obj(&b, statname, sizeof(struct stat64),
               (unsigned char *)&file_stat);

      free(file_content);
    }
  }

  if (stdin_content_filename) {
    FILE *fp;
    struct stat64 file_stat;
    char filename[6] = "stdin";
    char statname[11] = "stdin-stat";

    if ((fp = fopen(stdin_content_filename, "r")) == NULL ||
        fstat64(0, &file_stat) < 0) {
      fprintf(stderr, "Failure opening %s\n", stdin_content_filename);
        print_usage_and_exit(argv[0]);
      }

      fseek(fp, 0L, SEEK_END);
      long nbytes = ftell(fp);
      fseek(fp, 0L, SEEK_SET);

      unsigned char *file_content, *fptr;
      if ((file_content = (unsigned char *)malloc(nbytes)) == NULL) {
        fprintf(stderr, "Memory allocation failure\n");
        exit(1);
      }

      int read_char;
      fptr = file_content;
      while ((read_char = fgetc(fp)) != EOF) {
        *fptr = (unsigned char)read_char;
        fptr++;
      }

      push_obj(&b, filename, nbytes, file_content);
      push_obj(&b, statname, sizeof(struct stat64),
               (unsigned char *)&file_stat);

      free(file_content);
  }

  if (stdout_content_filename) {
    FILE *fp;
    struct stat64 file_stat;
    unsigned char file_content[1024];
    char filename[7] = "stdout";
    char statname[12] = "stdout-stat";

    if ((fp = fopen(stdout_content_filename, "r")) == NULL ||
        fstat64(1, &file_stat) < 0) {
      fprintf(stderr, "Failure opening %s\n", stdout_content_filename);
      print_usage_and_exit(argv[0]);
    }

    int read_char;
    for (int i = 0; i < 1024; ++i) {
      read_char = fgetc(fp);
      if (read_char == EOF) {
        file_content[i] = 0;
      } else {
        file_content[i] = (unsigned char)read_char;
      }
    }

    push_obj(&b, filename, 1024, file_content);
    push_obj(&b, statname, sizeof(struct stat64), (unsigned char *)&file_stat);
  }

  push_range(&b, "model_version", 1);

  if (!kTest_toFile(&b, "file.bout"))
    assert(0);
  return 0;
}
