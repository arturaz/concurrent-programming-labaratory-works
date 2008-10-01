#define N 2
#define M 3
#define TOTAL_THREADS N+M

#include <iostream>
#include <fstream>
#include <omp.h>
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

int main() {
    book_list book_lists[N];
    filter_list filter_lists[M];

    ifstream *in = new ifstream;
    in->open("SlajusA.txt");

    for (int i = 0; i < N; i++) {
        read_books(book_lists[i], in);
        print_books(book_lists[i], "p");
    }

    for (int i = 0; i < M; i++) {
        read_filters(filter_lists[i], in);
        print_filters(filter_lists[i], "p");
    }

    in->close();
    delete in;

    cout << "Entering parallel...\n";
    #pragma omp parallel num_threads(TOTAL_THREADS)
    {
        switch (omp_get_thread_num()) {
            case 0:
                print_books(book_lists[0], "0");
                break;
            case 1:
                print_books(book_lists[1], "1");
                break;
            case 2:
                print_filters(filter_lists[0], "0");
                break;
            case 3:
                print_filters(filter_lists[1], "1");
                break;
            case 4:
                print_filters(filter_lists[2], "2");
                break;
        }
    }  /* end of parallel section */


    return 0;
}
