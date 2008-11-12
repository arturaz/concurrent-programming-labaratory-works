
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
    
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        Thread p1 = new Thread(new Producer('X'));
        p1.setName("Prod1");
        p1.start();
        Thread p2 = new Thread(new Producer('Y'));
        p2.setName("Prod2");
        p2.start();
        for (int i = 0; i < consumersTotal; i++) {
            Thread c = new Thread(new Consumer());
            c.setName("Cons" + (i + 1));
            c.start();
        }
    }
    
    static void consumerFinished() throws InterruptedException {
        consumersFinishedLock.acquire();
        consumersFinished++;
        consumersFinishedLock.release();
    }

    static boolean allConsumersFinished() throws InterruptedException {
        boolean res;
        consumersFinishedLock.acquire();
        res = (consumersFinished == consumersTotal);
        consumersFinishedLock.release();
        return res;
    }
    
    static void debug(String msg) {
        System.err.println(Thread.currentThread().getName() + ":" + msg);
    }
}
class Storage {
    private static char letter;
    private static Semaphore lock = new Semaphore(1);
    private static Semaphore canWrite = new Semaphore(SlajusA_L2g.minReads);
    private static Semaphore canRead = new Semaphore(0);

    static char readUnless(char current) throws InterruptedException {
        lock.acquire();
        // Skaitymo procesai du kartus tos pacios reiksmes neskaito.
        if (current != letter) {
            current = letter;
            // Nauja simboli galima rasyti, kai sena perskaite bent 2 skaitymo
            // procesai.
            canWrite.release();
        }
        lock.release();
        return current;
    }
    
    static void store(char l) throws InterruptedException {
        // Nauja simboli galima rasyti, kai sena perskaite bent 2 skaitymo
        // procesai.
        canWrite.acquire(SlajusA_L2g.minReads);
        
        lock.acquire();
        // If we store same letter as there was before, consumers won't eat it
        // and we'll be in deadlock.
        if (letter == l) {
            SlajusA_L2g.debug("Tried to store same letter, releasing canWrite.");
            canWrite.release(SlajusA_L2g.minReads);
        }
        else {
            SlajusA_L2g.debug("Stored letter " + l + ".");
            letter = l;
        }
        //canRead.release();
        lock.release();
    }
}

class Consumer implements Runnable {
    private int read = 0;
    private char current;

    public void run() {
        SlajusA_L2g.debug("Running consumer.");
        char newCurrent;
        try {
            while (read < 10) {
                // Skaitymo procesai du kartus tos pacios reiksmes neskaito.
                SlajusA_L2g.debug("Consuming (current: " + current + 
                        ", read: " + read + ").");
                newCurrent = Storage.readUnless(current);
                if (newCurrent != current) {
                    SlajusA_L2g.debug("Got new one: " + newCurrent);
                    current = newCurrent;
                    System.out.print(current);
                    System.out.flush();
                    read++;
                }
                SlajusA_L2g.debug("Consumed.");
            }
        }
        catch (InterruptedException e) {}
        System.out.println(".");
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
    
    public Producer(char letter) {
        this.letter = letter;
    }

    public void run() {
        SlajusA_L2g.debug("Running producer.");
        // Darbas baigiamas, kai skaitymo procesai isves i ekrana po 10 
        // simboliu.
        try {
            while (! SlajusA_L2g.allConsumersFinished()) {
                SlajusA_L2g.debug("Storing letter: " + letter);
                Storage.store(letter);
                SlajusA_L2g.debug("Storing letter: " + letter + ". DONE.");
            }
        }
        catch (InterruptedException e) {}
        SlajusA_L2g.debug("Producer finished.");
    }
    
}
