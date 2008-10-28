#define N 2
#define M 3
#define TOTAL_THREADS N+M
#define DEBUG_ON false
#define DEBUG_STREAM cout
#define debug if (DEBUG_ON) DEBUG_STREAM
#define info if (DEBUG_ON) print_info(); if (DEBUG_ON) DEBUG_STREAM

#include <iostream>
#include <fstream>
#include <vector>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

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
    if (l != NULL) {
      omp_destroy_lock(l);
      delete l;
      l = NULL;
    }
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
 * Counting semaphore.
 */
class semaphore { // {{{
private:
  int count;
  omp_lock can_acquire;
  omp_lock main_lock;
public:
  semaphore() {
    semaphore(0);
  }
  
  semaphore(int count) {
    this->count = count;
    if (count == 0)
      can_acquire.acquire();
  }

  void acquire() {
    can_acquire.acquire();
    main_lock.acquire();
    count--;
    if (count > 0) can_acquire.release();
    main_lock.release();  
  }

  void release() {
    main_lock.acquire();
    count++;
    if (count > 0) can_acquire.release();
    main_lock.release();
  }
}; // }}}

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
    printf("%-4d | %-5d | %-3d\n", year, count, consumed);
  }

  /**
   * How much does this filter want to consume?
   */
  unsigned int get_wants_to_consume() {
    unsigned int quantity = count - consumed;
    return quantity;
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
    bool exausted = count == consumed;
    return exausted;
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
  semaphore can_read;
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
      add_book(b);
    }
    lock.release();
    can_read.release();
  }

  // Sorted add book to books.
  void add_book(book b) {
    for (vector<book>::iterator it = books.begin(); it < books.end(); it++) {
      if (it->year > b.year) {
        books.insert(it, b);
        return;
      }
    }

    // If we haven't inserted it yet, push it at the end.
    books.push_back(b);
  }

  /**
   * Consume a book by filter.
   *
   * Return number of items consumed.
   */
  unsigned int consume(filter &f) {
    can_read.acquire();
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

      f.consume(consumed);
    }
    else {
      can_read.release();
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
      print_header();
      int i = 1;
      for (vector<book>::iterator it = books.begin(); it < books.end(); it++) {
        printf("%-3d | ", i);
        it->print();
        i++;
      }
      print_line();
    }
  }
  
  void print_header() {
    printf("%-3s | %-31s | %-10s | %-4s | %-3s\n", "Nr.", "Title", "Printing", "Year", "Count");
    print_line();
  }

  void print_line() {
    cout << "------------------------------------------------------------------\n";
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
    int i = 1;
    print_header();
    for (vector<book>::iterator it=books.begin(); it < books.end(); it++) {
      printf("%-3d | ", i);
      it->print();
      i++;
    }
    print_line();
  }

  void print_header() {
    printf("%-3s | %-31s | %-10s | %-4s | %-3s\n", "Nr.", "Title", "Printing", "Year", "Count");
    print_line();
  }

  void print_line() {
    cout << "------------------------------------------------------------------\n";
  }

  void run() {
    for (vector<book>::iterator it=books.begin(); it < books.end(); it++) {
      data->store(*it);
      info << "Stored: ";
      if (DEBUG_ON) it->print();
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
  
  /**
   * Print all filters to stdout.
   */
  void print() {
    int i = 1;
    print_header();
    for (vector<filter>::iterator it=filters.begin(); it < filters.end(); it++) {
      printf("%-3d | ", i);
      it->print();
      i++;
    }
    print_line();
  }
  
  void print_header() {
    printf("%-3s | %-4s | %-5s | %-3s\n", "Nr.", "Year", "Count", "Consumed");
  }

  void print_line() {
    cout << "------------------------------------------\n";
  }

  void run() {
    bool should_finish_next_loop = false;
    while (true) {
      for (vector<filter>::iterator it = filters.begin(); it < filters.end(); it++) {
        if (! it->is_exausted()) {
          if (data->consumer_can_finish()) {
            info << "Consumer finished working because no producers and data are left.\n";
            return;
          }

          unsigned int consumed = data->consume(*it);
          info << "consuming: wants " << it->count << ", consumed " << consumed << " for filter " 
            << it->year << endl;
        }
      }

      if (data->get_producers_left() == 0) {
        if (should_finish_next_loop) {
          info << "Consumer finished working because no producers are left.\n";
          return;
        }
        info << "Consumer will finish next loop because no producers are left.\n";
        should_finish_next_loop = true;
      }
    }
  }
}; // }}}

/**
 * 2-4 lab. darbuose iš masyvuose S1(k1), S2(k2), ..., Sn(kn) surašytų duomenų 
 * pildomas vienas bendras sutvarkytas masyvas B(k) (tvarkymo raktas – 
 * pasirinktas požymis). Pildymo metu „panašūs” elementai sujungiami, visiems 
 * elementams įvedus papildomą lauką „kiekis”. Masyvuose V1(r1), V2(r2), ..., 
 * Vm(rm) surašyti kiekiai ir požymiai, kuriuos turintys duomenys iš bendrojo 
 * masyvo yra naudojami (atimant kiekius ir pašalinant, jei kiekis=0). 
 * 
 * Kiekvienas procesas, pildantis bendrąjį masyvą, į reikiamą jo vietą vieną 
 * duomenų porciją iš atitinkamo duomenų masyvo užrašo pats arba perduoda 
 * aptarnaujančiam procesui (tuo atveju užrašo aptarnaujantis procesas). Visi 
 * procesai pradeda darbą tuo pačiu metu. Naudojimo procesai turi dirbti tol, 
 * kol dar yra dirbančių procesų, kurie gali įdėti jiems reikalingų duomenų.
 * 
 * 2 lab. darbas: semaforai ir blokuotės. Gijos rašo į bendrą masyvą (šalina 
 * iš bendro masyvo). Realizuojama kritinių sekcijų apsauga ir sąlyginė 
 * sinchronizacija.
 * 
 * n=2
 * m=3
 * knygos pavadinimas, tiražas, išleidimo metai
 * @author Artūras Šlajus, IFF-6
 */
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
    producers[i].print();
  }

  for (int i = 0; i < M; i++) {
    consumers[i].set_name("Naudoti", i + 1);
    consumers[i].read(in);
    consumers[i].set_storage(data);
    consumers[i].print();
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
