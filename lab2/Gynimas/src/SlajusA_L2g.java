import java.util.concurrent.Semaphore;

/**
 * Artūras Šlajus, 3-čias kompiuteris
 * @author s95140
 */
public class SlajusA_L2g {
    public static int minReads = 2;
    public static int consumersTotal = 3;
    private static Semaphore consumersFinishedLock = new Semaphore(1);
    private static int consumersFinished = 0;
    private static Thread p1, p2;
    
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        Storage s = new Storage();
        p1 = new Thread(new Producer(s, 'X'));
        p1.setName("Prod1");
        p1.start();
        p2 = new Thread(new Producer(s, 'Y'));
        p2.setName("Prod2");
        p2.start();
        for (int i = 0; i < consumersTotal; i++) {
            Thread c = new Thread(new Consumer(s, i));
            c.setName("Cons" + (i + 1));
            c.start();
        }
    }
    
    static void consumerFinished() throws InterruptedException {
        consumersFinishedLock.acquire();
        consumersFinished++;
        // Signal producers that consumer has finished
        p1.interrupt();
        p2.interrupt();
        consumersFinishedLock.release();
    }

    static boolean allConsumersFinished() throws InterruptedException {
        boolean res;
        consumersFinishedLock.acquire();
        res = (consumersFinished == consumersTotal);
        consumersFinishedLock.release();
        return res;
    }
    
    // How much permits should we drain? Min 2, but if only 1 consumer is left,
    // then 1 or deadlock occurs otherwise.
    static int shouldAcquire() throws InterruptedException {
        consumersFinishedLock.acquire();
        int consumersLeft = consumersTotal - consumersFinished;
        int res = (consumersLeft < minReads) ? consumersLeft : minReads;
        consumersFinishedLock.release();
        return res;
    }
    
    static void debug(String msg) {
        //System.err.println(Thread.currentThread().getName() + ":" + msg);
    }
}
class Storage {
    private char letter;
    private Semaphore lock = new Semaphore(1);
    private Semaphore canWrite = new Semaphore(SlajusA_L2g.minReads);
    private Semaphore canRead[] = null;

    public Storage() {
        // Sukuriam po viena semafora kiekvienam vartotojui, kad uzblokuotu
        // skaityma, jeigu dar neirasyta nauja reiksme.
        canRead = new Semaphore[SlajusA_L2g.consumersTotal];
        for (int i = 0; i < SlajusA_L2g.consumersTotal; i++)
            canRead[i] = new Semaphore(0);
    }

    char read(int i) throws InterruptedException {
        // Skaitymo procesai du kartus tos pacios reiksmes neskaito.
        SlajusA_L2g.debug("acquiring canRead permit.");
        for (Semaphore s: canRead) 
            SlajusA_L2g.debug(s.availablePermits() + ", ");
        canRead[i].acquire();
        SlajusA_L2g.debug("acquired canRead permit.");
        
        SlajusA_L2g.debug("acquiring lock permit.");
        lock.acquire();
        SlajusA_L2g.debug("acquired lock permit.");
        char current = letter;
        // Nauja simboli galima rasyti, kai sena perskaite bent 2 skaitymo
        // procesai.
        SlajusA_L2g.debug("releasing canWrite permit.");
        canWrite.release();
        SlajusA_L2g.debug("released canWrite permit (avail: "
                + canWrite.availablePermits() + ").");
        
        SlajusA_L2g.debug("releasing lock permit.");
        lock.release();
        SlajusA_L2g.debug("released lock permit.");
        return current;
    }
    
    void store(char l) throws InterruptedException {
        // Nauja simboli galima rasyti, kai sena perskaite bent 2 skaitymo
        // procesai.
        try {
            SlajusA_L2g.debug("acquiring canWrite permit (avail: " 
                    + canWrite.availablePermits() + ", wants: " 
                    + SlajusA_L2g.shouldAcquire() + ")");
            canWrite.acquire(SlajusA_L2g.shouldAcquire());
        }
        catch (InterruptedException e) {
            // Retry storing, because some consumer finished working.
            store(l);
            return;
        }
        SlajusA_L2g.debug("acquired canWrite permit.");
        
        SlajusA_L2g.debug("acquiring lock permit.");
        lock.acquire();
        SlajusA_L2g.debug("acquired lock permit.");
        
        SlajusA_L2g.debug("Stored letter " + l + ".");
        letter = l;

        // Skaitymo procesai du kartus tos pacios reiksmes neskaito.         
        for (Semaphore s: canRead) {
            s.drainPermits();
            s.release();
        }
        SlajusA_L2g.debug("Drained & released canRead permits.");
        
        SlajusA_L2g.debug("releasing lock permit.");
        lock.release();
        SlajusA_L2g.debug("released lock permit.");
    }
}
class Consumer implements Runnable {
    private int read = 0;
    private char current;
    private int index;
    private Storage storage;

    public Consumer(Storage s, int index) {
        this.storage = s;
        this.index = index;
    }

    public void run() {
        SlajusA_L2g.debug("Running consumer.");
        try {
            while (read < 10) {
                // Skaitymo procesai du kartus tos pacios reiksmes neskaito.
                SlajusA_L2g.debug("Consuming (current: " + current + 
                        ", read: " + read + ").");
                current = this.storage.read(index);
                SlajusA_L2g.debug("Got : " + current);
                System.out.print(current);
                System.out.flush();
                read++;
                SlajusA_L2g.debug("Consumed.");
            }
        }
        catch (InterruptedException e) {}
        System.out.print(".");
        System.out.flush();
        // Darbas baigiamas, kai skaitymo procesai isves i ekrana po 10 
        // simboliu.
        try {
            SlajusA_L2g.debug("Reporting that consumer finished.");
            SlajusA_L2g.consumerFinished();
            SlajusA_L2g.debug("Reporting that consumer finished DONE.");
        }
        catch (InterruptedException e) {}
        SlajusA_L2g.debug("Consumer finished.");
    }
    
}
class Producer implements Runnable {
    private char letter;
    private Storage storage;
    
    public Producer(Storage s, char letter) {
        this.storage = s;
        this.letter = letter;
    }

    public void run() {
        SlajusA_L2g.debug("Running producer.");
        // Darbas baigiamas, kai skaitymo procesai isves i ekrana po 10 
        // simboliu.
        try {
            while (! SlajusA_L2g.allConsumersFinished()) {
                SlajusA_L2g.debug("Storing letter: " + letter);
                this.storage.store(letter);
                SlajusA_L2g.debug("Storing letter: " + letter + ". DONE.");
            }
        }
        catch (InterruptedException e) {}
        SlajusA_L2g.debug("Producer finished.");
    }
    
}
