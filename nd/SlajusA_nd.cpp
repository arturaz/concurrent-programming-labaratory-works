#define debug_on false
#define extradebug_on false
#define ifdebug if (debug_on) 
#define ifextradebug if (extradebug_on) 
#define debug ifdebug cout << "rank " << rank << " | "
#define extradebug ifextradebug cout << "rank " << rank << " | "
#define TAG_RESULT 0
#define TAG_A 1
#define TAG_B 2
#define TAG_BLOCKSPERROW 3
#define TAG_CELLSPERBLOCK 4
#define TAG_STATUS 5
#define TAG_LENGTH 6
#define STATUS_PROCEED 0
#define STATUS_ABORT 1

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
using namespace std;

void print_block(int *block, int length) { // {{{
  for (int brow = 0; brow < length; brow++) {
    for (int bcol = 0; bcol < length; bcol++) {
      printf("%-7d ", block[brow * length + bcol]);
    }
    cout << endl;
  }
} // }}}

class Matrix { // {{{  
private:
  int* data;
  int length;
  int numblocks;
  int blocksPerRow;
  int cellsPerBlock;
  int blockLength;
  int cellsPerBlockRow;

  void createStorage(int length, int numblocks) {
    this->length = length;
    int size = pow(length, 2);
    this->data = new int[size];

    this->numblocks = numblocks;
    this->cellsPerBlock = size / numblocks;
    this->blockLength = sqrt(cellsPerBlock);
    this->blocksPerRow = length / blockLength;
    this->cellsPerBlockRow = blockLength * length;
  }

  void fillZeroes() {
    // Surasom 0.
    for (int row = 0; row < length; row++) {
      for (int col = 0; col < length; col++) {
        this->set(row, col, 0);
      }
    }
  }
public:
  Matrix(int length) {
    createStorage(length, pow(length, 2));
    fillZeroes();
  }

  Matrix(int length, int numblocks) {
    createStorage(length, numblocks);
    fillZeroes();
  }

  Matrix(int length, int data[], int numblocks) {
    createStorage(length, numblocks);

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

  int getCellsPerBlock() {
    return cellsPerBlock;
  }

  int getBlocksPerRow() {
    return blocksPerRow;
  }

  int get(int row, int col) {
    return get(row * this->length + col);
  }

  int get(int index) {
    return this->data[index];
  }

  int* getBlock(int row, int col) {
    int *ret = new int[cellsPerBlock];
    for (int brow = 0; brow < blockLength; brow++) {
      for (int bcol = 0; bcol < blockLength; bcol++) {
        int retIndex = brow * blockLength + bcol;
        int getIndex = row * cellsPerBlockRow + brow * length + col * blockLength + bcol;
        /*
        printf("ret[%d * %d + %d = %d] = get(%d * %d + %d * %d + %d * %d + %d = %d)\n",
          brow, blockLength, bcol, retIndex,
          row, cellsPerBlockRow, brow, length, col, blockLength, bcol, getIndex
        );
        */
        ret[retIndex] = get(getIndex);
      }
    }
    return ret;
  }

  void set(int row, int col, int value) {
    set(row * this->length + col, value);
  }

  void set(int index, int value) {
    this->data[index] = value;
  }

  void setBlock(int row, int col, int* block) {
    for (int brow = 0; brow < blockLength; brow++) {
      for (int bcol = 0; bcol < blockLength; bcol++) {
        int blockIndex = brow * blockLength + bcol;
        int setIndex = row * cellsPerBlockRow + brow * length + col * blockLength + bcol;
        set(setIndex, block[blockIndex]);
      }
    }
  }

  void print() {
    for (int i = 0; i < length; i++) {
      for (int j = 0; j < length; j++) {
        printf("%-7d ", this->get(i, j));
      }
      cout << endl;
    }
    cout << endl;
  }

  void printBlock(int row, int col) {
    cout << "Block @ " << row << "," << col << ":\n";
    int* block = getBlock(row, col);
    print_block(block, blockLength);
    delete block;
    cout << endl;
  }


  void printBlocks() {
    for (int row = 0; row < blocksPerRow; row++)
      for (int col = 0; col < blocksPerRow; col++) 
        printBlock(row, col);
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
    // UCS col i by i (for all cols)
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

}; // }}}

int getNumBlocks(int numcells, int numprocs) { // {{{
    int iTo = sqrt(numcells) / 2;

    for (int i = 1; i <= iTo; i++) {
      int cellsInBlock = i * i;
      int numblocks = numcells / (i * i);
      if (cellsInBlock * numblocks == numcells) {
        if (numprocs >= numblocks + 1) {
          cout << "Using " << numblocks << " blocks with " << cellsInBlock 
            << " cells in block. Total of " << (numblocks + 1) << " processes.\n";
          return numblocks;
        }
        else {
          cout << "Skipping " << numblocks << " blocks with " << cellsInBlock 
            << " cells in block. Total of " << (numblocks + 1) << " processes."
            << " We only have " << numprocs << " processes.\n";
        }
      }
    }

    if (numprocs < 2)
      return -1;
    else {
      cout << "Using single block with " << numcells
        << " cells in block. Total of 2 processes.\n";
      return 1;
    }
} // }}}

void send_status(int status, int from, int to) {
  for (int i = from; i <= to; i++)
    MPI::COMM_WORLD.Send(&status, 1, MPI::INT, i, TAG_STATUS);
}

void send_status(int status, int numprocs) {
  send_status(status, 1, numprocs);
}

void mpi_exit(int status) {
  MPI::Finalize();
  exit(status);
}

int main(int argc, char** argv) {
  // Start parallel.
  MPI::Init();

  int rank = MPI::COMM_WORLD.Get_rank();
  int numprocs = MPI::COMM_WORLD.Get_size();

  // Controller
  // {{{
  if (rank == 0) {
    int length, lengthB;

    ifstream inA(argv[1]);
    inA >> length;
    int numcells = length * length;
    int numblocks = getNumBlocks(numcells, numprocs);
    if (numblocks == -1) {
      cerr << "We cannot compute these matrixes with this process count!\n";
      cerr << "Quiting.\n";
      send_status(STATUS_ABORT, numprocs);
      exit(1);
    }

    int dataA[numcells];
    cout << "Reading into dataA, length: " << length << "... ";
    for (int i = 0; i < numcells; i++) {
      extradebug << "Reading [" << i << "]... ";
      inA >> dataA[i];
      extradebug << "Done. Have read:" << dataA[i] << "\n";
    }
    cout << "Done.\n";
    inA.close();

    ifstream inB(argv[2]);
    int dataB[numcells];
    inB >> lengthB;

    if (length != lengthB) {
      cerr << "Matrixes A and B must be same length!\n";
      cerr << "Quiting.\n";
      send_status(STATUS_ABORT, numprocs);
      exit(2);
    }

    cout << "Reading into dataB, length: " << lengthB << "... ";
    for (int i = 0; i < numcells; i++) {
      extradebug << "Reading [" << i << "]... ";
      inB >> dataB[i];
      extradebug << "Done. Have read:" << dataB[i] << "\n";
    }
    cout << "Done.\n";
    inB.close();

    debug << "INIT\n";
    send_status(STATUS_PROCEED, 1, numblocks);
    // Terminate unneeded processes.
    send_status(STATUS_ABORT, numblocks + 1, numprocs - 1);

    Matrix a(length, dataA, numblocks);
    Matrix b(length, dataB, numblocks);
    // For results
    Matrix res(length, numblocks);

    debug << "Initial matrixes:\n";
    debug << "Initial matrix A:\n";
    ifdebug a.print();
    ifextradebug {
      debug << "Initial matrix A blocks:\n";
      a.printBlocks();
    }
    debug << "Initial matrix B:\n";
    ifdebug b.print();
    ifextradebug {
      debug << "Initial matrix B blocks:\n";
      b.printBlocks();
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

    double start = MPI_Wtime();
    int cellsPerBlock = a.getCellsPerBlock();
    int blockLength = sqrt(cellsPerBlock);
    int blocksPerRow = a.getBlocksPerRow();
    // Send params to procs.
    for (int i = 1; i <= numblocks; i++) {
      MPI::COMM_WORLD.Send(&length, 1, MPI::INT, i, TAG_LENGTH);
      MPI::COMM_WORLD.Send(&blocksPerRow, 1, MPI::INT, i, TAG_BLOCKSPERROW);
      MPI::COMM_WORLD.Send(&cellsPerBlock, 1, MPI::INT, i, TAG_CELLSPERBLOCK);
    }

    // Send data to procs.
    for (int shift = 0; shift < length; shift++) {
      // Sending stuff {{{
      for (int row = 0; row < blocksPerRow; row++) {
        for (int col = 0; col < blocksPerRow; col++) {
          int sendTo = row * blocksPerRow + col + 1;

          debug << "getting data for " << row << "," << col << endl;
          int *blockA = a.getBlock(row, col);
          int *blockB = b.getBlock(row, col);

          ifdebug {
            debug << "A:\n";
            a.printBlock(row, col); 
            debug << "B:\n";
            b.printBlock(row, col); 
          }

          debug << "sending blockA to " << sendTo << endl;
          MPI::COMM_WORLD.Send(blockA, cellsPerBlock, MPI::INT, sendTo, TAG_A);
          debug << "sent blockA" << endl;

          debug << "sending blockB to " << sendTo << endl;
          MPI::COMM_WORLD.Send(blockB, cellsPerBlock, MPI::INT, sendTo, TAG_B);
          debug << "sent blockB" << endl;

          delete blockA;
          delete blockB;
        }
      } // }}}

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
    // {{{
    int *block = new int[cellsPerBlock];
    for (int row = 0; row < blocksPerRow; row++) {
      for (int col = 0; col < blocksPerRow; col++) {
        int recvFrom = row * blocksPerRow + col + 1;

        debug << "Receiving result from " << recvFrom << "...\n";
        MPI::COMM_WORLD.Recv(block, cellsPerBlock, MPI::INT, recvFrom, TAG_RESULT);
        debug << "Received result from " << recvFrom << ":\n";
        ifdebug print_block(block, blockLength);

        debug << "Setting result from " << recvFrom << " to " << row << "," << col << "\n";
        res.setBlock(row, col, block);
      }
    }
    delete block;
    // }}}
    double end = MPI_Wtime();

    ifdebug {
      debug << "Got:\n";
      res.print();
    }
    cout << "Took: " << (end - start) << "s\n";
    printf("%-2s %-10d %-5.8f %-7d %-7d %-7d\n", "?", numblocks + 1, (end - start), length * length, numblocks, cellsPerBlock);

    mpi_exit(0);
  }
  // }}}
  // Receivers
  // {{{
  else {
    debug << "INIT\n";
    int status;
    debug << "Waiting for status...\n";
    MPI::COMM_WORLD.Recv(&status, 1, MPI::INT, 0, TAG_STATUS);
    debug << "Received status: " << status << endl;
    if (status == STATUS_ABORT) {
      debug << "Aborting...\n";
      mpi_exit(0);
    }
    debug << "Proceeding...\n";

    int length;
    debug << "Receiving length\n";
    MPI::COMM_WORLD.Recv(&length, 1, MPI::INT, 0, TAG_LENGTH);
    debug << "Received:" << length << "\n";

    int blocksPerRow;
    debug << "Receiving blocksPerRow\n";
    MPI::COMM_WORLD.Recv(&blocksPerRow, 1, MPI::INT, 0, TAG_BLOCKSPERROW);
    debug << "Received:" << blocksPerRow << "\n";

    int cellsPerBlock;
    debug << "Receiving cellsperblock\n";
    MPI::COMM_WORLD.Recv(&cellsPerBlock, 1, MPI::INT, 0, TAG_CELLSPERBLOCK);
    debug << "Received:" << cellsPerBlock << "\n";

    int blockLength = sqrt(cellsPerBlock);

    // Initialize storage.
    int *block = new int[cellsPerBlock];
    for (int i = 0; i < cellsPerBlock; i++)
      block[i] = 0;

    // Wait for all data and sum it into cell.
    int *blockA = new int[cellsPerBlock];
    int *blockB = new int[cellsPerBlock];
    for (int shift = 0; shift < length; shift++) {
      debug << "receiving blockA for " << shift << endl;
      MPI::COMM_WORLD.Recv(blockA, cellsPerBlock, MPI::INT, 0, TAG_A);
      debug << "received blockA:" << endl;
      ifdebug print_block(blockA, blockLength); 

      debug << "receiving blockB for " << shift << endl;
      MPI::COMM_WORLD.Recv(blockB, cellsPerBlock, MPI::INT, 0, TAG_B);
      debug << "received blockB:" << endl;
      ifdebug print_block(blockB, blockLength); 

      debug << "Adding to current:\n";
      for (int i = 0; i < cellsPerBlock; i++)
        block[i] += blockA[i] * blockB[i];
      ifdebug print_block(block, blockLength);
    }
    delete blockA;
    delete blockB;

    // Send back the result.
    debug << "sending back result\n";
    MPI::COMM_WORLD.Send(block, cellsPerBlock, MPI::INT, 0, TAG_RESULT);
    debug << "sent result\n";
    delete block;

    mpi_exit(0);
  }
  // }}}

}
