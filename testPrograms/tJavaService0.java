import com.yahoo.pluton.Service;

class tJavaService0 {
    public static void main(String[] args) throws Throwable {
        System.err.println("Java Test Service Started");
	com.yahoo.pluton.Service s = new com.yahoo.pluton.Service("test Java");
	System.err.println("Service API Version=" + s.getAPIVersion());
	if (s.getAPIVersion().length() == 0) throw new Exception("API Length must be > 0");
	s.initialize();
	if (s.hasFault()) throw new Error("Unexpected fault setting");
	if (s.getFaultCode() != 0) throw new Error("Non-zero fault code");
	boolean gotdupe = false;
	try {
	    s.initialize();
	}
	catch (Exception e) {
	    System.err.println("Caught dupe init - good");
	    gotdupe = true;
	}

	if (!gotdupe) throw new Error("Duplication initialization not detected");

	if (!s.hasFault()) throw new Error("Unexpected non-fault setting");
	if (s.getFaultCode() == 0) throw new Error("Zero fault code");
	String str = s.getFaultMessage("A prefix", true);
	if (str.length() == 0) throw new Error("FaultMessage is zero length");
	System.err.println("Fault message=" + str);

	if (!s.getRequest()) throw new Error("First getRequest failed");

	System.err.println("ServiceKey=" + s.getServiceKey());
	System.err.println("ServiceApplication=" + s.getServiceApplication());
	System.err.println("ServiceFunction=" + s.getServiceFunction());
	int v = s.getServiceVersion();
	System.err.println("ServiceVersion=" + v);
	System.err.println("SerializationType=" + s.getSerializationType());
	System.err.println("ClientName=" + s.getClientName());

	String ctx = s.getContext("ctx");
	System.err.println("ctx=" + ctx);

	byte[] r = s.getRequestDataBytes();
	s.sendResponseBytes(r);

	while (true) {
	    System.err.println("Getting next Request");
	    if (!s.getRequest()) break;
	    String sr = s.getRequestData();
	    System.err.println("Have request: " + sr);
	    s.sendResponse(sr);
	}

	System.err.println("Terminating");
	s.terminate();
	System.err.println("Exiting");
    }
}
