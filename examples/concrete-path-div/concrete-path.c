
int my_islower(char *p) { return 20 / (p[0] - 65); }

int main(int argc, char **argv) {
  if (argc > 1 && argv[1][1] == 't') {
    my_islower(argv[1]);
    return 0;
  }
  return 1;
}
