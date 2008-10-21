#define N 2
#define M 3
#define TOTAL_THREADS N+M
#define DEBUG_ON false
#define DEBUG_STREAM cerr
#define debug if (DEBUG_ON) DEBUG_STREAM
#define info if (DEBUG_ON) print_info(); if (DEBUG_ON) DEBUG_STREAM

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

  /**
   * Is this book container empty?
   */
  bool empty() {
    return count == 0;
  }

  bool operator==(book &b) {
    return (this->title == b.title) && (this->printing == b.printing) 
      && (this->year == b.year) && (this->count == b.count);
  }
}; // }}}

class filter { // {{{
private:
  unsigned int consumed;
public:
  unsigned int year;
  unsigned int count;

  filter() {
    consumed = 0;
  }
  
  /**
   * Print to stdout.
   */
  void print() {
    printf("Year: %-4d | Count: %-3d | Consumed: %-3d\n", year, count, consumed);
  }

  /**
   * How much does this filter want to consume?
   */
  unsigned int get_wants_to_consume() {
    return count - consumed;
  }

  /**
   * Record that we have consumed count i.
   */
  void consume(unsigned int i) {
    consumed += i;
    // Safety trigger.
    if (consumed > count)
      consumed = count;
  }

  /**
   * Have we consumed everything we need?
   */
  bool is_exausted() {
    return count == consumed;
  }
}; // }}}

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
  omp_lock producer_lock;
  unsigned int producer_count;
public:
  storage(int count) {
    producer_count = count;
  }

  /**
   * Reduce working producer count by 1.
   */
  void producer_finished() {
    producer_lock.acquire();
    producer_count -= 1;
    producer_lock.release();
  }

  /**
   * How many producers left working?
   */
  unsigned int get_producers_left() {
    producer_lock.acquire();
    unsigned int i = producer_count;
    producer_lock.release();
    return i;
  }

  /**
   * Can consumer finish?
   */
  bool consumer_can_finish() {
    producer_lock.acquire();
    lock.acquire();
    bool can_finish = (producer_count == 0) && (books.size() == 0);
    lock.release();
    producer_lock.release();
    return can_finish;
  }

  /**
   * Store book into storage.
   */
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

  /**
   * Consume a book by filter.
   *
   * Return number of items consumed.
   */
  unsigned int consume(filter &f) {
    lock.acquire();
    unsigned int consumed = 0;
    book *b = find_by_year(f.year);
    if (b != NULL) {
      unsigned int wants_to_consume = f.get_wants_to_consume();
      if (b->count < wants_to_consume)
        consumed = b->count;
      else
        consumed = wants_to_consume;

      b->count -= consumed;
      if (b->empty()) {
        debug << "Deleting " << b->title << endl;
        remove(b);
      }
    }
    lock.release();
    return consumed;
  }

  /**
   * Find first book by given year.
   */
  book* find_by_year(unsigned int year) {
    for (vector<book>::iterator it = books.begin(); it < books.end(); it++) {
      if (it->year == year) {
        return &(*it);
      }
    }

    return NULL;
  }

  /**
   * Remove book from storage.
   */
  void remove(book* b) {
    for (vector<book>::iterator it = books.begin(); it < books.end(); it++) {
      if (*b == *it) {
        books.erase(it);
        return;
      }
    }    
  }

  /**
   * Print everything to stdout.
   */
  void print() {
    if (books.size() == 0) {
      cout << "No data.\n";
    }
    else {
      for (vector<book>::iterator it = books.begin(); it < books.end(); it++) {
        it->print();
      }
    }
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
  void print_info() {
    DEBUG_STREAM << "[" << name << "] ";
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
      info << "Stored: ";
      it->print();
    }

    data->producer_finished();
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
    while (true) {
      for (vector<filter>::iterator it = filters.begin(); it < filters.end(); it++) {
        if (! it->is_exausted()) {
          if (data->consumer_can_finish())
            return;

          unsigned int consumed = data->consume(*it);
          info << "Consuming " << consumed << " from " << it->count << " for filter " 
            << it->year << "\n";
          it->consume(consumed);
        }
      }

      if (data->get_producers_left() == 0) {
        return;
      }
    }
  }
}; // }}}

int main(int argc, char *argv[]) {
  producer producers[N];
  consumer consumers[M];
  storage *data = new storage(N);

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

  #pragma omp barrier
  cout << "Galutiniai rezultatai:\n";
  data->print();

  delete data;

  return 0;
}

// vim:tabstop=2 shiftwidth=2
