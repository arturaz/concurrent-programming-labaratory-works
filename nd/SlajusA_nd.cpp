#define debug_on true
#define ifdebug if (debug_on) 
#define debug ifdebug cout << "rank " << rank << " | "
#define TAG_RESULT 0
#define TAG_A 1
#define TAG_B 2

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
using namespace std;

class Matrix { // {{{
public:
  Matrix(int length) {
    createStorage(length);

    // Surasom 0.
    for (int row = 0; row < length; row++) {
      for (int col = 0; col < length; col++) {
        this->set(row, col, 0);
      }
    }
  }

  Matrix(int length, int data[]) {
    createStorage(length);

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
    return get(row * this->length + col);
  }

  int get(int index) {
    return this->data[index];
  }

  void set(int row, int col, int value) {
    set(row * this->length + col, value);
  }

  void set(int index, int value) {
    this->data[index] = value;
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

  void leftCircularShift() {
    for (int i = 0; i < length; i++)
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

  void upCircularShift() {
    for (int i = 0; i < length; i++)
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

  void createStorage(int length) {
    this->length = length;
    this->data = new int[length ^ 2];
  }
}; // }}}

int main(int argc, char** argv) {
  int dataA[] = {
    1,2,3,
    4,5,6,
    7,8,9
  };
  int dataB[] = {
    1,2,3,
    4,5,6,
    7,8,9
  };
  int expected[] = {
      30,    36,    42,
      66,    81,    96,
     102,   126,   150
  };

  int length = 3;
  int size = length * length;

  // Start parallel.
  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0) {
    sleep(0.1);

    debug << "INIT\n";

    Matrix a(length, dataA);
    Matrix b(length, dataB);

    // For results
    Matrix exp(3, expected);
    Matrix res(3);

    ifdebug {
      debug << "Initial matrix:\n";
      a.print();
    }

    // Skew matrices
    a.horizontalSkew();
    b.verticalSkew();

    ifdebug {
      debug << "A after skewing:\n";
      a.print();
      debug << "B after skewing:\n";
      b.print();
    }

    // Send data to procs.
    for (int shift = 0; shift < length; shift++) {
      for (int index = 0; index < size; index++) {
        int sendTo = index + 1;

        debug << "getting data for index " << index << endl;
        int cellA = a.get(index);
        int cellB = b.get(index);

        ifdebug {
          debug << "A:\n";
          a.print(); 
          debug << "B:\n";
          b.print();
        }

        debug << "got a: " << cellA << ", b: " << cellB << endl;

        debug << "sending cellA (" << cellA << ") to " << sendTo << endl;
        MPI_Send(&cellA, 1, MPI_INT, sendTo, TAG_A, MPI_COMM_WORLD);
        debug << "sent cellA" << endl;

        ifdebug {
          debug << "A:\n";
          a.print(); 
          debug << "B:\n";
          b.print();
        }

        debug << "sending cellB (" << cellB << ") to " << sendTo << endl;
        MPI_Send(&cellB, 1, MPI_INT, sendTo, TAG_B, MPI_COMM_WORLD);
        debug << "sent cellB" << endl;
        
        ifdebug {
          debug << "A:\n";
          a.print(); 
          debug << "B:\n";
          b.print();
        }
      }

      debug << "Shifting matrixes...\n";
      a.leftCircularShift();
      b.upCircularShift();

      ifdebug {
        debug << "A:\n";
        a.print();
        debug << "B:\n";
        b.print();
      }
    }

    // Receive all results from processes and set the results.
    MPI_Status stat;
    for (int index = 0; index < size; index++) {
      int cell = 0;
      MPI_Recv(&cell, 1, MPI_INT, index + 1, TAG_RESULT, MPI_COMM_WORLD, &stat);
      res.set(index, cell);
    }

    cout << "Expected:\n";
    exp.print();
    cout << "Got:\n";
    res.print();
  }
  else {
    debug << "INIT\n";
    int cell = 0;

    // Wait for all data and sum it into cell.
    MPI_Status stat;
    for (int shift = 0; shift < length; shift++) {
      int a, b;
      debug << "receiving cellA" << endl;
      MPI_Recv(&a, 1, MPI_INT, 0, TAG_A, MPI_COMM_WORLD, &stat);
      debug << "received cellA (" << a << ")" << endl;

      debug << "receiving cellB" << endl;
      MPI_Recv(&b, 1, MPI_INT, 0, TAG_B, MPI_COMM_WORLD, &stat);
      debug << "received cellB (" << b << ")" << endl;

      cell += a * b;
      debug << "Cell now is: " << cell << endl;
    }

    // Send back the result.
    MPI_Send(&cell, 1, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD);
  }

  MPI_Finalize();
}
