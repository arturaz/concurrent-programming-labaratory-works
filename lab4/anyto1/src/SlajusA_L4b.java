
import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.Collection;
import org.jcsp.lang.Alternative;
import org.jcsp.lang.AltingChannelInput;
import org.jcsp.lang.CSProcess;
import org.jcsp.lang.Channel;
import org.jcsp.lang.ChannelOutput;
import org.jcsp.lang.Guard;
import org.jcsp.lang.One2OneChannel;
import org.jcsp.lang.Parallel;

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
 * 4 lab. darbas: pranešimų perdavimas. Visi procesai, turintys duomenis, 
 * savo masyvų elementus siunčia papildomam aptarnaujančiam procesui, kuris 
 * atlieka rašymą ir šalinimą.
 * 
 * n=2
 * m=3
 * knygos pavadinimas, tiražas, išleidimo metai
 * @author Artūras Šlajus, IFF-6
 */
public class SlajusA_L4b {    
    /**
     * Main app.
     * 
     * @param args
     */    
    public static void main(String[] args) throws IOException {
        if (args.length == 1)
            new SlajusA_L4a(args[0]);
        else
            new SlajusA_L4a("SlajusA.txt");
    }

    public SlajusA_L4a(String fileName) throws IOException {
        new App(fileName);
    }
}

/**
 * Class for simplyfying communication means of alternating channels.
 * @author Artūras
 */
class CCPool {
    private ArrayList<CommunicationChannel> pool = 
            new ArrayList<CommunicationChannel>();
    
    void extend(Collection<CommunicationChannel> collection) {
        App.debug("Extending comm channels from collection (size: " + 
                collection.size() + "). Current pool size: " + pool.size());
        pool.addAll(collection);
        App.debug("Extended comm channels from collection (size: " + 
                collection.size() + "). Current pool size: " + pool.size());
    }
    
    int getTypeByIndex(int index) {
        return pool.get(index).getType();
    }
    
    CommunicationChannel get(int index) {
        return pool.get(index);
    }

    Guard[] getGuards() {
        App.debug("Generating guards, pool size: " + pool.size());
        Guard[] guard = new Guard[pool.size()];
        int i = 0;
        for (CommunicationChannel c: pool) {
            guard[i] = c.in();
            i++;
        }

        return guard;
    }
}


/**
 * Two way communication channel.
 * @author Artūras
 */
class CommunicationChannel {
    private One2OneChannel channel;
    private AltingChannelInput in;
    private ChannelOutput out;
    private int type;
    
    public final static int PRODUCER = 1;
    public final static int CONSUMER = 2;
    
    CommunicationChannel(int type) {
        // Create channel of immunity 0.
        // (fcking nerds that made this package. Feels like MMO)
        this.channel = Channel.one2one();
        this.in = channel.in();
        this.out = channel.out();
        this.type = type;
    }
    
    AltingChannelInput in() {
        return in;
    }
    
    ChannelOutput out() {
        return out;
    }

    public int getType() {
        return type;
    }
    
    /**
     * Send a query. Send Packet to out and then wait and return for the answer.
     * @param type
     * @param data
     * @return
     */
    public Packet query(int type, Object data) {
        write(type, data);
        return read();
    }
    
    Packet query(int type) {
        return query(type, 0);
    }
    
    public void write(int type, Object data) {
        App.debug("Writing packet to channel (type: " + type + ")");
        out.write(new Packet(type, data));
        App.debug("Written packet to channel (type: " + type + ")");
    }
    
    void write(int type) {
        write(type, 0);
    }

    Packet read() {
        return (Packet) in.read();
    }
}
class App {
    /**
     * Number of threads.
     */
    public static int producerCount = 2;
    public static int consumerCount = 3;
    
    Storage storage = new Storage();
    Producer[] producers = new Producer[producerCount];
    Consumer[] consumers = new Consumer[consumerCount];
    static double startTime = System.currentTimeMillis();
    Parallel processes = new Parallel();

    public App(String inputFilename) throws IOException {
        readData(inputFilename);
        printData();
        
        processes.addProcess(storage);
        processes.run();
        
        System.out.println("Bendras masyvas:");
        storage.writeDataTo(System.out);
        return;
    }

    /**
     * Print out all data in nice tables.
     */
    public void printData() {
        for (Producer l: producers) {
            l.writeDataTo(System.out);
            System.out.println("");
        }
        for (Consumer l: consumers) {
            l.writeDataTo(System.out);
            System.out.println("");
        }
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

        // Create producers/consumers
        CommunicationChannel c;
        for (int i = 0; i < producerCount; i++) {
            c = new CommunicationChannel(CommunicationChannel.PRODUCER);
            producers[i] = new Producer(in, i, c);
            storage.addProducerChannel(c);
        }
        processes.addProcess(producers);
        
        for (int i = 0; i < consumerCount; i++) {
            c = new CommunicationChannel(CommunicationChannel.CONSUMER);
            consumers[i] = new Consumer(in, i, c);
            storage.addConsumerChannel(c);
        }
        processes.addProcess(consumers);

        in.close();
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

// Abstract class for record lists.
abstract class RecordList implements CSProcess {
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
    public String format = "%2d | %-30s | %10d | %4d | %d";

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



/**
 * Class for transmiting data packets through Channels.
 * @author Artūras
 */
class Packet {
    public final static int PRODUCER_DONE = 1;
    public final static int BOOK = 2;
    public final static int GET_PRODUCERS_LEFT = 3;
    public final static int CONSUMER_DONE = 4;
    public final static int FILTER = 5;

    private int type;
    private Object data;

    public Packet(int type, Object data) {
        this.type = type;
        this.data = data;
    }

    public Object getData() {
        return data;
    }

    public int getType() {
        return type;
    }
}
class Producer extends RecordList {        
    /**
     * Format of the title
     */
    public String format = "%2s | %-30s | %-10s | %-4s | %s";

    protected Book[] data;
    protected CommunicationChannel channel;

    public Producer(BufferedReader in, int number, CommunicationChannel c)
    throws IOException {
        super(in, number);
        this.channel = c;
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
        return String.format(format, "Nr", "Title", "Printing", "Year", "Count") + 
                "\n------------------------------------------------------------------";
    }

    public void writeDataTo(PrintStream out) {
        super.writeDataTo(out, data);
    }        

    public void run() {
        Thread.currentThread().setName("Papildyti" + number);
        
        App.debug("Iterating through books");
        for (Book book: data) {
            App.debug("[Producing] " + book.toString());
            channel.write(Packet.BOOK, book);
            App.debug("[Produced] " + book.toString());
        }
        App.debug("Signaling that producer is done.");
        channel.write(Packet.PRODUCER_DONE, "Producer done.");
        App.debug("Exiting.");
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
    
    public void exausted() {
        count = 0;
    }
    
    public boolean isExausted() {
        return count == 0;
    }
}

class Consumer extends RecordList {
    protected Filter[] data;
    protected CommunicationChannel channel;

    /**
     * Format of the title
     */
    public String format = "%2s | %4s | %s";

    public Consumer(BufferedReader in, int number, CommunicationChannel c)
            throws IOException {
        super(in, number);
        this.channel = c;
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
        return String.format(format, "Nr", "Year", "Count") +
                "\n------------------";
    }

    public void writeDataTo(PrintStream out) {
        super.writeDataTo(out, data);
    }

    public void run() {
        Thread.currentThread().setName("Vartoti" + number);
        boolean canFinishNextLoop = false;
        while (true) {
            for (Filter filter: data) {
                if (! filter.isExausted()) {
                    App.debug("[trying to consume] " + filter.toString());
                    Packet p = channel.query(Packet.FILTER, filter);
                    Book book = (Book) p.getData();
                    App.debug("[GOT] " + book);
                    if (book != null) {
                        App.debug("[Consumed] " + filter.toString());
                    }
                }
            }
            
            App.debug("Getting how many producers are left.");
            Packet p = channel.query(Packet.GET_PRODUCERS_LEFT);
            App.debug("Got producers left: " + p.getData());
            if ((Integer) p.getData() == 0) {
                // Ensure that all possible data is consumed by having 2 
                // iterations when all consumers are done.
                if (canFinishNextLoop) {
                    App.debug("Messaging that consumer finished.");
                    channel.write(Packet.CONSUMER_DONE);
                    return;
                }
                canFinishNextLoop = true;
            }
        }
    }
}

/**
 * Bendras sutvarkytas masyvas, kurį naudos abi klasės.
 */
class Storage implements CSProcess {
    private ArrayList<Book> data = new ArrayList<Book>();
    private ArrayList<CommunicationChannel> producerChannels = 
            new ArrayList<CommunicationChannel>();
    private ArrayList<CommunicationChannel> consumerChannels = 
            new ArrayList<CommunicationChannel>();

    void addProducerChannel(CommunicationChannel c) {
        producerChannels.add(c);
    }
    
    void addConsumerChannel(CommunicationChannel c) {
       consumerChannels.add(c);
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

    /**
     * Adds a book into array. Increments count for existing book if there 
     * is one with same year as in book.
     * @param book
     */
    public Book produce(Book book) {
        Book existing = findByYear(book.getYear());
        if (existing == null) {
            addBook(book);
        }
        else {
            book = existing;
            book.count += 1;
        }

        return book;
    }
    
    /**
     * Sorted add book to data.
     * @param book
     */
    private void addBook(Book book) {
        int index = 0;
        for (Book b: data) {
            if (book.getYear().compareTo(b.getYear()) > -1) {
                data.add(index, book);
                return;
            }
            index++;
        }
        
        // Add to end elsewhere
        data.add(book);
    }
    
    /**
     * Deletes book from list.
     * @param book
     */
    public void delete(Book book) {
        data.remove(book);
    }

    /**
     * Consume book by year.
     * 
     * @param year
     * @return
     */
    public Book consume(Filter filter) {
        int year = filter.getYear();
        int count = filter.getCount();
        App.debug("Finding by YEAR " + year);
        Book book = findByYear(year);
        if (book != null) {
            if (book.count >= count) {
                filter.exausted();
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
        
        return book;
    }    
    
    /**
     * Format of the title
     */
    public String format = "%2s | %-30s | %-10s | %-4s | %s";
    
    /**
     * Prints data out in a nice table.
     */
    public void writeDataTo(PrintStream out) {
        if (data == null || data.size() == 0) {
            out.println("No data.");
        }
        else {
            out.println(
                    String.format(format, "Nr", "Title", "Printing", "Year", "Count") + 
                    "\n------------------------------------------------------------------"
            );
            for (Object o: data) {
                out.println(o.toString());
            }
        }
    }

    public void run() {        
        Thread.currentThread().setName("Saugykla");
        CCPool pool = new CCPool();
        pool.extend(producerChannels);
        pool.extend(consumerChannels);
        
        Guard[] guards = pool.getGuards();
        App.debug("Guards size: " + guards.length);
        for (Guard g: guards)
            App.debug("Guard " + g.toString());
        
        Alternative alt = new Alternative(guards);
        int producersRunning = producerChannels.size();
        int consumersRunning = consumerChannels.size();
        App.debug("Entering main loop (prods: " + producersRunning + ", cons: "
                + consumersRunning + ")");
        while (producersRunning > 0 || consumersRunning > 0) {
            App.debug("Selecting channel...");
            int index = alt.fairSelect();
            App.debug("Selected channel " + index);
            
            CommunicationChannel chan = pool.get(index);
            Packet p = chan.read();
            
            switch (pool.getTypeByIndex(index)) {
                case CommunicationChannel.PRODUCER:
                    App.debug("Message by producer.");
                    switch (p.getType()) {
                        case Packet.BOOK:
                            App.debug("DATA.");
                            produce((Book) p.getData());
                            break;
                        case Packet.PRODUCER_DONE:
                            App.debug("Producer finished.");
                            producersRunning--;
                            break;
                    }
                    break;
                    
                case CommunicationChannel.CONSUMER:
                    App.debug("Message by consumer.");
                    switch(p.getType()) {
                        case Packet.FILTER:
                            App.debug("DATA.");
                            Book b = consume((Filter) p.getData());
                            chan.write(Packet.BOOK, b);
                            break;
                        case Packet.GET_PRODUCERS_LEFT:
                            App.debug("Get producers left.");
                            chan.write(Packet.GET_PRODUCERS_LEFT, producersRunning);
                            break;
                        case Packet.CONSUMER_DONE:
                            consumersRunning--;
                            App.debug("Received msg that consumer finished " +
                                    "(left: " + consumersRunning + ")");
                            break;
                    }
                    break;
            }
        }
        App.debug("Finished main loop.");
    }
}
    
