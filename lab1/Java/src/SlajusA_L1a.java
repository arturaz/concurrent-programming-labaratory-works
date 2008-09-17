
import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintStream;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * n=2
 * m=3
 * knygos pavadinimas, tiražas, išleidimo metai
 * @author Artūras Šlajus, IFF-6
 */
public class SlajusA_L1a {
    class App {
        BookList[] bookLists = new BookList[bookThreadCount];
        FilterList[] filterLists = new FilterList[filterThreadCount];
        
        public App(String inputFilename) throws IOException {            
            readData(inputFilename);            
            printData();
            startThreads();
            return;
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
                Logger.getLogger(SlajusA_L1a.class.getName()).log(Level.SEVERE, null, ex);
                return;
            }
            
            // Create appenders
            for (int i = 0; i < bookThreadCount; i++) {
                bookLists[i] = new BookList(in, i);
            }
            for (int i = 0; i < filterThreadCount; i++) {
                filterLists[i] = new FilterList(in, i);
            }
            
            in.close();
        }
        
        /**
         * Print out all data in nice tables.
         */
        public void printData() {
            for (BookList l: bookLists) {
                l.writeDataTo(System.out);
            }
            for (FilterList l: filterLists) {
                l.writeDataTo(System.out);
            }
        }
        
        /**
         * Start all threads.
         */
        public void startThreads() {
            for (BookList l: bookLists) {
                new Thread(l).start();
            }
            for (FilterList l: filterLists) {
                new Thread(l).start();
            }
        }
    }
    
    /**
     * Number of threads.
     */
    public static int bookThreadCount = 2;
    public static int filterThreadCount = 3;
    
    /**
     * Abstract class for Records.
     */
    public abstract class Record {
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
    
    public abstract class RecordList implements Runnable {
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
    public class Book extends Record {
        /**
         * Format of output line.
         */
        public String format = "%-10s | %2d | %-30s | %10d | %4d";
        
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
            return String.format(format, Thread.currentThread().getName(), 
                    getId(), title, printing, year);
        }
    }
    
    public class BookList extends RecordList {        
        /**
         * Format of the title
         */
        public String format = "%-10s | %2s | %-30s | %-10s | %-4s";
        
        protected Book[] data;

        public BookList(BufferedReader in, int number) throws IOException {
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
            writeDataTo(System.out);
        }
    }
    
    /**
     * Class for [year, count] records.
     */
    public class Filter extends Record {
        /**
         * Format of output line.
         */
        public String format = "%-10s | %2d | %4d | %d";
        
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
            return String.format(format, Thread.currentThread().getName(), 
                    getId(), year, count);
        }
    }

    public class FilterList extends RecordList {
        protected Filter[] data;
        
        /**
         * Format of the title
         */
        public String format = "%-10s | %2s | %4s | %s";
        
        public FilterList(BufferedReader in, int number) throws IOException {
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
            writeDataTo(System.out);
        }
    }
    
    /**
     * Main app.
     * 
     * @param args
     */    
    public static void main(String[] args) throws IOException {
        new SlajusA_L1a();
    }

    /**
     * OMGHAX because someone is lazy.
     */
    public SlajusA_L1a() throws IOException {
        new App("SlajusA.txt");
    }
}
