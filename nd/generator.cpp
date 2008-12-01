#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
using namespace std;

void generate(char *fname, int length) {
  ofstream out(fname);

  int size = length * length;
  out << length << "\n";
  for (int i = 0; i < size; i++) {
    out << (random() / 10000000) << " ";
  }
  out.close();
}

int main(int argc, char** argv) {
  int length = atoi(argv[1]);

  char name[100];
  sprintf(name, "dataA.%d", length);
  generate(name, length);
  sprintf(name, "dataB.%d", length);
  generate(name, length);

  return 0;
}
