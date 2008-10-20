#define N 2
#define M 3
#define TOTAL_THREADS N+M

#include <iostream>
#include <fstream>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

struct book {
  char title[32];
  unsigned int printing;
  unsigned int year;
  // Additional attribute for grouping.
  unsigned int count;
};

class book_list {
public:
  book *data;
  unsigned int length;

  book_list() {
    data = NULL;
    length = 0;
  }

  void init(unsigned int length) {
    data = new book[length];
    this->length = length;
  }

  ~book_list() {
    if (data != NULL)
      delete data;
  }
};

struct filter {
  unsigned int year;
  unsigned int count;
};

class filter_list {
public:
  filter *data;
  unsigned int length;

  filter_list() {
    data = NULL;
    length = 0;
  }

  void init(unsigned int length) {
    data = new filter[length];
    this->length = length;
  }

  ~filter_list() {
    if (data != NULL)
      delete data;
  }

};

/**
 * Simple class for storing name.
 */
class named_class { // {{{
private:
  string name;
public:
  /**
   * Set name.
   */
  void set_name(string name, int index) {
    char ic[2];
    sprintf(ic, "%d", index);
    this->name = name + string(ic);
  }

  /**
   * Get name
   */
  string get_name() {
    return name;
  }
}; // }}}

/**
 * Class for producing data and storing it in shared array.
 */
class producer : public named_class { // {{{
private:
  book_list books;

public:
  producer() {}

  /**
   * Create producer and read books from in.
   */
  producer(ifstream *in) {
    producer();
    read(in);
  }

  /**
   * Read books from ifstream into list.
   */
  void read(ifstream *in) {
    // Read length
    unsigned int length;
    (*in) >> length;
    books.init(length);
    in->get();

    for (int i = 0; i < books.length; i++) {
      in->get(books.data[i].title, 31);
      *in >> books.data[i].printing;
      *in >> books.data[i].year;
      books.data[i].count = 1;
      in->get();
    }

    in->get();
  }

  void run() {
    //cout << "yay\n";
    //cout << get_name();
  }
}; // }}}

/**
 * Class for consuming data.
 */
class consumer : public named_class { // {{{
private:
  filter_list filters;
public:
  consumer() {}

  consumer(ifstream *in) {
    consumer();
    read(in);
  }  

  /**
   * Read filters from ifstream.
   */
  void read(ifstream *in) {
    // Read length
    unsigned int length;
    (*in) >> length;
    filters.init(length);
    in->get();

    for (int i = 0; i < filters.length; i++) {
      *in >> filters.data[i].year;
      *in >> filters.data[i].count;
      in->get();
    }

    in->get();
  }

  void run() {
    //cout << "yayc\n";
    //cout << get_name();
  }
}; // }}}

/**
 * Array-like storage for producers and consumers.
 *
 * Provides locking.
 */
class storage { // {{{
private:
  book_list books;
  omp_lock_t* lock;
public:
  storage() {
    omp_init_lock(lock);
  }

  ~storage() {
    omp_destroy_lock(lock);
  }
}; // }}}

void print_books(book_list &list, string desc) {
  printf("%-10s | %-2s | %-30s | %-10s | %-4s\n", "TID", "Nr", "Title", "Printing", "Year");
  for (int i = 0; i < list.length; i++) {
    string name("Papildyti");
    name += desc;
    printf("%-10s | %-2d | %-30s | %-10d | %-4d\n", name.c_str(), i + 1,
        list.data[i].title,
        list.data[i].printing,
        list.data[i].year
        );
    int amax = rand() * 1000;
    for (int a = 0; a < amax; a++)
      amax ^ a;
  }
  cout << "\n";
}

void print_filters(filter_list &list, string desc) {
  printf("%-10s | %-4s | %-s\n", "TID", "Nr", "Year", "Count");
  for (int i = 0; i < list.length; i++) {
    string name("Naudoti");
    name += desc;
    printf("%-10s | %-2d | %-4d | %-d\n", name.c_str(), i + 1,
        list.data[i].year,
        list.data[i].count
        );
    int amax = rand() * 1000;
    for (int a = 0; a < amax; a++)
      amax ^ a;
  }
  cout << "\n";
}

int main(int argc, char *argv[]) {
  producer producers[N];
  consumer consumers[M];
  storage storage;

  ifstream *in = new ifstream;
  if (argc == 2) {
    in->open(argv[1]);
  }
  else {
    in->open("SlajusA.txt");
  }

  for (int i = 0; i < N; i++) {
    producers[i].set_name("Papildyti", i + 1);
    producers[i].read(in);
  }

  for (int i = 0; i < M; i++) {
    consumers[i].set_name("Naudoti", i + 1);
    consumers[i].read(in);
  }

  in->close();
  delete in;

  #pragma omp parallel num_threads(TOTAL_THREADS)
  {
    cout << "switch\n";
    switch (omp_get_thread_num()) {
      case 0:
        cout << "case0\n";
        // Vien del to, kad shita iskvieciu, programa segfaultinas :(
        producers[0].run();
        break;
      case 1:
        cout << "case1\n";
        //producers[1].run();
        break;
      case 2:
        cout << "case2\n";
        //consumers[0].run();
        break;
      case 3:
        cout << "case3\n";
        //consumers[1].run();
        break;
      case 4:
        cout << "case4\n";
        //consumers[2].run();
        break;
    }
  }

  return 0;
}

// vim:tabstop=2 shiftwidth=2
