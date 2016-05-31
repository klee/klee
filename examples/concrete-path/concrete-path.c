
int my_islower(char *p) {
  int y[50];
  return y[p[0]];
}

int main(int argc, char **argv) {
  if (argc > 1 && argv[1][1] == 't') {
    my_islower(argv[1]);
    return 0;
  }
  return 1;
}
