#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
using namespace std;

class Matrix {
public:
  Matrix(int length, int data[]) {
    this->length = length;
    this->data = new int[length * length];

    // Perpilam duomenis.
    for (int row = 0; row < length; row++) {
      for (int col = 0; col < length; col++) {
        this->set(row, col, data[row * length + col]);
      }
    }
  }
  
  ~Matrix() {
    delete this->data;
  }

  int get(int row, int col) {
    return this->data[row * this->length + col];
  }

  void set(int row, int col, int value) {
    this->data[row * this->length + col] = value;
  }

  void print() {
    for (int i = 0; i < length; i++) {
      for (int j = 0; j < length; j++) {
        printf("%-4d ", this->get(i, j));
      }
      cout << endl;
    }
    cout << endl;
  }

  void horizontalSkew() {
    // LCS row i by i (for all rows)
    for (int i = 0; i < length; i++)
      for (int j = 0; j < i; j++)
        leftCircularShift(i);
  }

  void leftCircularShift(int row) {
    int first = get(row, 0);
    for (int i = 0; i < length - 1; i++)
      set(row, i, get(row, i + 1));
    set(row, length - 1, first);
  }

  void verticalSkew() {
    // UCS column i by i (for all columns)
    for (int i = 0; i < length; i++)
      for (int j = 0; j < i; j++)
        upCircularShift(i);
  }

  void upCircularShift(int col) {
    int first = get(0, col);
    for (int i = 0; i < length - 1; i++)
      set(i, col, get(i + 1, col));
    set(length - 1, col, first);
  }

private:
  int* data;
  int length;
};

int data[] = {
  1,2,3,
  4,5,6,
  7,8,9
};

int main() {
  Matrix m(3, data);
  m.print();
  m.horizontalSkew();
  m.print();
  m.verticalSkew();
  m.print();
}
