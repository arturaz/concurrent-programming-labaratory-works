#define N 2
#define M 3
#define TOTAL_THREADS N+M
#define DEBUG_ON true
#define debug if (DEBUG_ON) cerr

#include <iostream>
#include <fstream>
#include <vector>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

class book { // {{{
public:
  char title[32];
  unsigned int printing;
  unsigned int year;
  // Additional attribute for grouping.
  unsigned int count;

  /**
   * Print a book to stdout.
   */
  void print() {
    printf("%-31s | %-10d | %-4d | %-3d\n", title, printing, year, count);
  }
}; // }}}

struct filter {
  unsigned int year;
  unsigned int count;
};

/**
 * Simple wrapper for OMP lock.
 */
class omp_lock { // {{{
private:
  omp_lock_t* l;
public:
  omp_lock() {
    l = new omp_lock_t;
    omp_init_lock(l);
  }

  ~omp_lock() {
    omp_destroy_lock(l);
    delete l;
  }

  /**
   * Acquire the lock. Wraps omp_set_lock.
   */
  void acquire() {
    omp_set_lock(l);
  }

  /**
   * Release the lock. Wraps omp_unset_lock.
   */
  void release() {
    omp_unset_lock(l);
  }
}; // }}}

/**
 * Array-like storage for producers and consumers.
 *
 * Provides locking.
 */
class storage { // {{{
private:
  vector<book> books;
  omp_lock lock;
public:
  void store(book &b) {
    lock.acquire();
    bool saved = false;
    for (vector<book>::iterator it = books.begin(); it < books.end(); it++) {
      if (it->year == b.year) {
        debug << "found existing book " << it->title << ", incrementing from " 
          << it->count << " to " << (it->count + b.count) << " by " << b.count << "\n";
        it->count += b.count;
        saved = true;
      }
    }

    if (! saved) {
      debug << "adding new book " << b.title << "\n";
      books.push_back(b);
    }
    lock.release();
  }
}; // }}}

/**
 * Both producers and consumers must extend this..
 */
class base_class { // {{{
private:
  string name;
protected:
  storage *data;
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

  /**
   * Print out info string.
   */
  void info(string s) {
    cout << "[" << name << "] " << s;
  }

  /**
   * Set storage.
   */
  void set_storage(storage *data) {
    this->data = data;
  }

}; // }}}

/**
 * Class for producing data and storing it in shared array.
 */
class producer : public base_class { // {{{
private:
  vector<book> books;

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
    in->get();

    for (int i = 0; i < length; i++) {
      book b;
      in->get(b.title, 31);
      *in >> b.printing;
      *in >> b.year;
      b.count = 1;
      books.push_back(b);
      in->get();
    }

    in->get();
  }

  /**
   * Print all books to stdout.
   */
  void print() {
    for (vector<book>::iterator it=books.begin(); it < books.end(); it++) {
      it->print();
    }
  }

  void run() {
    for (vector<book>::iterator it=books.begin(); it < books.end(); it++) {
      data->store(*it);
      info("Stored: ");
      it->print();
    }
  }
}; // }}}

/**
 * Class for consuming data.
 */
class consumer : public base_class { // {{{
private:
  vector<filter> filters;
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
    in->get();

    for (int i = 0; i < length; i++) {
      filter f;
      *in >> f.year;
      *in >> f.count;
      filters.push_back(f);
      in->get();
    }

    in->get();
  }

  void run() {
  }
}; // }}}

int main(int argc, char *argv[]) {
  producer producers[N];
  consumer consumers[M];
  storage *data = new storage;

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
    producers[i].set_storage(data);
  }

  for (int i = 0; i < M; i++) {
    consumers[i].set_name("Naudoti", i + 1);
    consumers[i].read(in);
    consumers[i].set_storage(data);
  }

  in->close();
  delete in;

  #pragma omp parallel num_threads(TOTAL_THREADS)
  {
    switch (omp_get_thread_num()) {
      case 0:
        producers[0].run();
        break;
      case 1:
        producers[1].run();
        break;
      case 2:
        consumers[0].run();
        break;
      case 3:
        consumers[1].run();
        break;
      case 4:
        consumers[2].run();
        break;
    }
  }

  delete data;

  return 0;
}

// vim:tabstop=2 shiftwidth=2
