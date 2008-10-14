
import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.concurrent.Semaphore;

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
public class SlajusA_L2a {    
    /**
     * Main app.
     * 
     * @param args
     */    
    public static void main(String[] args) throws IOException {
        new SlajusA_L2a();
    }

    public SlajusA_L2a() throws IOException {
        new App("SlajusA.txt");
    }
}

class App {    
    /**
     * Number of threads.
     */
    public static int producerCount = 2;
    public static int consumerCount = 3;
    
    Producer[] producers = new Producer[producerCount];
    Consumer[] consumers = new Consumer[consumerCount];
    static OrderedArray data = new OrderedArray(producerCount);
    static Semaphore producersDone = new Semaphore(producerCount);
    static double startTime = System.currentTimeMillis();

    public App(String inputFilename) throws IOException {
        readData(inputFilename);
        startThreads();
        return;
    }
    
    public static double getPassedTime() {
        return System.currentTimeMillis() - startTime;
    }
    
    public static void debug(String s) {
        System.out.println(String.format("%4.0fms [ %-10s ] -- %s",
                getPassedTime(),
                Thread.currentThread().getName(),
                s));
    }

    /**
     * Read data from given filename.
     * @param inputFilename
     */
    public void readData(String inputFilename) throws IOException {
        BufferedReader in;
        try {
            in = new BufferedReader(new FileReader(inputFilename));           
        } catch (FileNotFoundException ex) {
            System.err.println("File not found: " + ex.getMessage());
            return;
        }

        // Create appenders
        for (int i = 0; i < producerCount; i++) {
            producers[i] = new Producer(in, i);
        }
        for (int i = 0; i < consumerCount; i++) {
            consumers[i] = new Consumer(in, i);
        }

        in.close();
    }

    /**
     * Start all threads.
     */
    public void startThreads() {
        for (Producer l: producers) {
            new Thread(l).start();
        }
        for (Consumer l: consumers) {
            new Thread(l).start();
        }
    }
}

/**
 * Abstract class for Records.
 */
abstract class Record {
    /**
     * ID.
     */
    private Integer id;

    Record(Integer id) {
        this.id = id;
    }

    public Integer getId() {
        return id;
    }
}

abstract class RecordList implements Runnable {
    protected int number;

    public RecordList(BufferedReader in, int number) throws IOException {
        readFromReader(in);
        this.number = number;
    }

    public int getNumber() {
        return number;
    }

    /**
     * Read data from BufferedReader.
     * @param in
     * @throws java.io.IOException
     */
    public void readFromReader(BufferedReader in) throws IOException {
        // Read the length of data.
        int length;        
        try {
            length = Integer.parseInt(in.readLine().trim());
        } catch (IOException ex) {
            length = 0;
        }

        // Read data.
        createDataStorage(length);
        for (int i = 0; i < length; i++) {
            setInDataStorage(i, in.readLine());
        }

        in.readLine();
    }

    /**
     * Prints data out in a nice table.
     */
    public void writeDataTo(PrintStream out, Object[] data) {
        if (data == null) {
            out.println("No data.");
        }
        else {
            out.println(getHeader());
            for (Object o: data) {
                out.println(o.toString());
            }
        }
    }

    /**
     * Create data storage of apropriate type.
     */
    public abstract void createDataStorage(int size);
    /**
     * Set value in data storage at index from line.
     * @param index
     * @param line
     */
    public abstract void setInDataStorage(int index, String line);
    /**
     * Return header row of the list
     * @return
     */
    public abstract String getHeader();
}


/**
 * Class for [title, printing, year] records.
 */
class Book extends Record implements Comparable<Book> {
    /**
     * Format of output line.
     */
    public String format = "%2d | %-30s | %10d | %4d | count: %d";

    /**
     * Title of the book.
     */
    private String title;
    /**
     * Printing of the book.
     */
    private Integer printing;
    /**
     * Year of release.
     */        
    private Integer year;
    /**
     * Count of the same books.
     */
    public Integer count;

    /**
     * Creates new record from input.
     * 
     * Input format: 30chars 10chars 4chars - title, printing, year
     * @param input data string
     */        
    public Book(Integer id, String input) {
        super(id);
        title = input.substring(0, 30);
        printing = Integer.parseInt(input.substring(30, 40).trim());
        year = Integer.parseInt(input.substring(40).trim());
        count = 1;
    }

    public Integer getPrinting() {
        return printing;
    }

    public String getTitle() {
        return title;
    }

    public Integer getYear() {
        return year;
    }

    @Override
    public String toString() {
        return String.format(format, getId(), title, printing, year, count);
    }

    @Override
    public int compareTo(Book book) {
        return getTitle().compareTo(book.getTitle());
    }
}
class Producer extends RecordList {        
    /**
     * Format of the title
     */
    public String format = "%-10s | %2s | %-30s | %-10s | %-4s";

    protected Book[] data;

    public Producer(BufferedReader in, int number) throws IOException {
        super(in, number);
    }

    @Override
    public void createDataStorage(int size) {
        data = new Book[size];
    }

    @Override
    public void setInDataStorage(int index, String line) {
        data[index] = new Book(index + 1, line);
    }

    @Override
    public String getHeader() {
        return String.format(format, "Thread", "Nr", "Title", "Printing", 
                "Year");
    }

    public void writeDataTo(PrintStream out) {
        super.writeDataTo(out, data);
    }        

    public void run() {
        Thread.currentThread().setName("Papildyti" + getNumber());
        try {
            for (Book book: data) {
                App.debug("[Producing] " + book.toString());
                book = App.data.produce(book);
                App.debug("[Produced] " + book.toString());
            }
            App.data.producerFinished();
        }
        catch (InterruptedException e) {}
    }
}

/**
 * Class for [year, count] records.
 */
class Filter extends Record {
    /**
     * Format of output line.
     */
    public String format = "%2d | %4d | %d";

    /**
     * Year of release.
     */        
    private Integer year;
    /**
     * Count.
     */
    private Integer count;

    public Filter(Integer id, String input) {
        super(id);
        year = Integer.parseInt(input.substring(0, 4).trim());
        count = Integer.parseInt(input.substring(5).trim());
    }

    public Integer getCount() {
        return count;
    }

    public Integer getYear() {
        return year;
    }

    @Override
    public String toString() {
        return String.format(format, getId(), year, count);
    }
}

class Consumer extends RecordList {
    protected Filter[] data;

    /**
     * Format of the title
     */
    public String format = "%-10s | %2s | %4s | %s";

    public Consumer(BufferedReader in, int number) throws IOException {
        super(in, number);
    }

    @Override
    public void createDataStorage(int size) {
        data = new Filter[size];
    }

    @Override
    public void setInDataStorage(int index, String line) {
        data[index] = new Filter(index + 1, line);
    }

    @Override
    public String getHeader() {
        return String.format(format, "Thread", "Nr", "Year", "Count");
    }

    public void writeDataTo(PrintStream out) {
        super.writeDataTo(out, data);
    }

    public void run() {
        Thread.currentThread().setName("Naudoti" + getNumber());
        try {
            while (! App.data.consumerCanFinish()) {
                for (Filter filter: data) {
                    App.debug("[trying to consume] " 
                            + filter.toString());
                    Book book = App.data.consume(filter);
                    App.debug("[GOT] " + book);
                    if (book != null) {
                        App.debug("[Consumed] " 
                                + filter.toString());
                    }
                }
            }
        }
        catch (InterruptedException e) {}
    }
}

/**
 * Bendras sutvarkytas masyvas, kurį naudos abi klasės.
 */
class OrderedArray {
    private ArrayList<Book> data = new ArrayList<Book>();
    public Semaphore lock = new Semaphore(1, true);
    private int producersWorking;

    public OrderedArray(int producersWorking) {
        this.producersWorking = producersWorking;
    }

    /**
     * Find book by year.
     * @param year
     * @return 
     */
    public Book findByYear(Integer year) {
        for (Book b: data) {
            if (b.getYear().equals(year)) {
                return b;
            }
        }

        return null;
    }
    
    public void producerFinished() throws InterruptedException {
        lock.acquire();
        producersWorking -= 1;
        lock.release();
    }

    /**
     * Adds a book into array. Increments count for existing book if there 
     * is one with same year as in book.
     * @param book
     */
    public Book produce(Book book) throws InterruptedException {
        App.debug("Aquiring lock.");
        lock.acquire();
        App.debug("Aquired lock.");
        Book existing = findByYear(book.getYear());
        if (existing == null) {
            data.add(book);
        }
        else {
            book = existing;
            book.count += 1;
        }

        Collections.sort(data);
        lock.release();
        App.debug("Released lock.");
        return book;
    }
    
    /**
     * Deletes book from list.
     * @param book
     */
    public void delete(Book book) {
        data.remove(book);
    }
    
    public boolean consumerCanFinish() throws InterruptedException {
        lock.acquire();
        App.debug("can finish called, producers working: " 
                + producersWorking + ", data size: " + data.size());
        boolean canFinish = (producersWorking == 0) && data.size() == 0;
        lock.release();
        return canFinish;
    }

    /**
     * Consume book by year.
     * 
     * @param year
     * @return
     */
    public Book consume(Filter filter) throws InterruptedException {
        lock.acquire();
        int year = filter.getYear();
        int count = filter.getCount();
        App.debug("Finding by YEAR " + year);
        Book book = findByYear(year);
        if (book != null) {
            if (book.count >= count) {
                book.count -= count;
                if (book.count == 0) {
                    delete(book);
                    App.debug("Deleted " + book.toString());
                }
                App.debug("READY FOR CONSUMING " + book.toString());
            }
            else {
                book = null;
            }
        }
        lock.release();
        return book;
    }
}
    
