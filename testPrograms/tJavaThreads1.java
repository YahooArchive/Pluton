import com.yahoo.pluton.Client;

public class tJavaThreads1
{
    private aThread1 at;
    public tJavaThreads1(String name)
    {
        at = new aThread1(name);
	at.start();
    }
    
    public void join() throws InterruptedException
    {
	at.join();
    }

    private class aThread1 extends Thread {
	aThread1(String name) {
	    _name = name;
	}

	private String _name;
        public void run()
	{
	    System.out.println(_name + " started");
	    com.yahoo.pluton.Client c = new com.yahoo.pluton.Client();
            for(int i=0;i<100;++i) {
		com.yahoo.pluton.ClientRequest r = new com.yahoo.pluton.ClientRequest();
		r.setRequestData(_name);
		c.addRequest("system.echo.0.raw", r);
		c.executeAndWaitAll();
		String resp = r.getResponseData();
		System.out.println("Response for " + _name + " is " + resp);
		if (!resp.equals(_name)) {
		    System.out.println("Unexpected response of " + resp);
		}
	    }
        }
    }
    public static void main(String args[]) throws Throwable
    {
        tJavaThreads1 t1 = new tJavaThreads1("t1");
        tJavaThreads1 t2 = new tJavaThreads1("t2");
        tJavaThreads1 t3 = new tJavaThreads1("t3");
        tJavaThreads1 t4 = new tJavaThreads1("t4");
        tJavaThreads1 t5 = new tJavaThreads1("t5");
        tJavaThreads1 t6 = new tJavaThreads1("t6");
        tJavaThreads1 t7 = new tJavaThreads1("t7");

	System.out.println("Waiting on t1"); t1.join();
	System.out.println("Waiting on t2"); t2.join();
	System.out.println("Waiting on t3"); t3.join();
	System.out.println("Waiting on t4"); t4.join();
	System.out.println("Waiting on t5"); t5.join();
	System.out.println("Waiting on t6"); t6.join();
	System.out.println("Waiting on t7"); t7.join();
    }
}
