#define N 2
#define M 3
#define TOTAL_THREADS 5

#include <iostream>
#include <fstream>
#include <mpi.h>
using namespace std;

struct book {
    char title[32];
    unsigned int printing;
    unsigned int year;
};

struct book_list {
    book *data;
    unsigned int length;
};

struct filter {
    unsigned int year;
    unsigned int count;
};

struct filter_list {
    filter *data;
    unsigned int length;
};

/**
 * Get MPI rank.
 */
int get_rank() {
  if (MPI::Is_initialized()) {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    return rank;
  } 
  else {
    return 0;
  }
}

char* get_cpu() {
  int namelen;
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  MPI_Get_processor_name(processor_name, &namelen);
  return processor_name;
}

/**
 * Read books from ifstream into list.
 */
void read_books(book_list &list, ifstream *in) {
    // Read length
    (*in) >> list.length;
    in->get();

    list.data = new book[list.length];
    for (int i = 0; i < list.length; i++) {
        in->get(list.data[i].title, 31);
        *in >> list.data[i].printing;
        *in >> list.data[i].year;
        in->get();
    }

    in->get();
}

/**
 * Read filters from ifstream into list.
 */
void read_filters(filter_list &list, ifstream *in) {
    // Read length
    (*in) >> list.length;
    in->get();

    list.data = new filter[list.length];
    for (int i = 0; i < list.length; i++) {
        *in >> list.data[i].year;
        *in >> list.data[i].count;
        in->get();
    }

    in->get();
}

void print_books(book_list &list, int num) {
    printf("%-15s | %-11s | %-2s | %-30s | %-10s | %-4s\n", "CPU", "TID", "Nr", 
        "Title", "Printing", "Year");
    for (int i = 0; i < list.length; i++) {
        printf("%-15s | Papildyti%-2d | %-2d | %-30s | %-10d | %-4d\n", 
                get_cpu(), 
                num,
                i + 1,
                list.data[i].title,
                list.data[i].printing,
                list.data[i].year
                );
    }
    cout << "\n";
}

void print_filters(filter_list &list, int num) {
    printf("%-15s | %-11s | %-2s | %-4s | %-s\n", "CPU", "TID", "Nr", "Year", "Count");
    for (int i = 0; i < list.length; i++) {
        printf("%-15s | Naudoti%-4d | %-2d | %-4d | %-d\n", get_cpu(), 
                num,
                i + 1,
                list.data[i].year,
                list.data[i].count
                );
    }
    cout << "\n";
}

int main(int argc, char *argv[]) {
    MPI::Init();

    book_list book_lists[N];
    filter_list filter_lists[M];

    ifstream *in = new ifstream;
    in->open("SlajusA.txt");

    for (int i = 0; i < N; i++) {
        read_books(book_lists[i], in);
        //if (get_rank() == 0)
        //    print_books(book_lists[i]);
    }

    for (int i = 0; i < M; i++) {
        read_filters(filter_lists[i], in);
        //if (get_rank() == 0)
        //    print_filters(filter_lists[i]);
    }

    in->close();
    delete in;

    switch(get_rank()) {
      case 0:
        print_books(book_lists[0], 1);
        break;
      case 1:
        print_books(book_lists[1], 2);
        break;
      case 2:
        print_filters(filter_lists[0], 1);
        break;
      case 3:
        print_filters(filter_lists[1], 2);
        break;
      case 4:
        print_filters(filter_lists[2], 3);
        break;
    }
    MPI::Finalize();

    return 0;
}
