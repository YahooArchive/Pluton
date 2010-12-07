import com.yahoo.pluton.Client;

class tJavaClient1 {
    public static void main(String[] args) throws Throwable {
        System.out.println("tJavaClient1 arg0=" + args[0]);
	com.yahoo.pluton.Client c0 = new com.yahoo.pluton.Client();
	com.yahoo.pluton.Client c1 = new com.yahoo.pluton.Client("Javaa HW", 3000);
	boolean badPath = false;
	try {
	    c1.initialize("/tmpp/badpath/xx");
	}
	catch (IllegalArgumentException e) {
	    System.out.println("Caught init error");
	    badPath = true;
	    if (!c1.hasFault()) throw new Exception("Client should have fault when init fails");
	    System.out.println("Client FaultCode = " + c1.getFaultCode());
	    if (c1.getFaultCode() == 0) throw new Exception("Client Fault code should not be zero");
	    System.out.println("FaultMessageLong=" + c1.getFaultMessage("FM prefix", true));
	    System.out.println("FaultMessageShort=" + c1.getFaultMessage("FM prefix", false));

	    c1.reset();
	    c1.initialize(args[0]);
	}
	if (!badPath) throw new Exception("Bad path not detected by Client::initialize()");

	//	c1.setDebug(true);
	System.out.println("Client API Version=" + c1.getAPIVersion());
	if (c1.getAPIVersion().length() == 0) throw new Exception("API length cannot be zero");

	c1.setTimeoutMilliSeconds(5000);
	if (c1.getTimeoutMilliSeconds() != 5000) throw new Exception("getTimeoutMilliSeconds");

	com.yahoo.pluton.ClientRequest r1 = new com.yahoo.pluton.ClientRequest();
	r1.reset();
	r1.setAttribute(com.yahoo.pluton.ClientRequest.noRemoteAttr);
	if (r1.getAttribute(com.yahoo.pluton.ClientRequest.noRemoteAttr)) {
	    System.out.println("noRemoteAttr set - good");
	}
	else {
	    throw new Exception("Failed set/get Attribute");
	}

	r1.setAttribute(com.yahoo.pluton.ClientRequest.needAffinityAttr);
	r1.clearAttribute(com.yahoo.pluton.ClientRequest.needAffinityAttr);
	if (r1.getAttribute(com.yahoo.pluton.ClientRequest.needAffinityAttr)) {
	    throw new Exception("Failed clear Attribute");
	}
	else {
	    System.out.println("needAffinityAttr not set - good");
	}

	// Catch an illegal context setting

	boolean caughtContext = false;
	try {
	    r1.setContext("pluton.noway", "23");
	}
	catch (Throwable e) {
	    System.out.println("Caught illegal context name");
	    caughtContext = true;
	}
	if (!caughtContext) throw new Exception("Context namespace restriction not caught");

	r1.setContext("echo.log", "1");
	r1.setContext("echo.sleepMS", "3000");
	
	System.out.println("one");
	byte b2[] = { 'T', 'h', 'i', 's', ' ', 's', 'h', 'o', 'u', 'l', 'd', ' ', 'c', 'o', 'm', 'e', ' ', 'b', 'a', 'c', 'k', ' ', 't', 'o', ' ', 'm' };
	System.out.println("two");

	r1.setRequestDataBytes(b2);
	System.out.println("three");

	r1.setClientHandle(1001);
	if (r1.getClientHandle() != 1001) throw new Exception("getHandle failed");
	r1.setClientHandle(1001);

	c1.addRequest("system.echo.0.raw", r1);
	if (c1.hasFault()) throw new Exception("Unexpected fault from Client");

	com.yahoo.pluton.ClientRequest rbad = new com.yahoo.pluton.ClientRequest();
	boolean caughtAdd = false;
	try {
	    c0.addRequest("system.echo.XXX.raw", rbad);
	}
	catch (IllegalArgumentException e) {
	    System.out.println("Caught bad Service Key");
	    caughtAdd = true;
	}
	if (!caughtAdd) throw new Exception("Invalid Service Key did not raise an exception");

	com.yahoo.pluton.ClientRequest r2 = new com.yahoo.pluton.ClientRequest();
	r2.setRequestData("Here is some request data for r2");
	r2.setClientHandle(1002);
	r2.setContext("echo.sleepMS", "2000");
	c1.addRequest("system.echo.0.raw", r2);

	com.yahoo.pluton.ClientRequest r3 = new com.yahoo.pluton.ClientRequest();
	r3.setRequestData("Here is some request data for r3");
	r3.setClientHandle(1003);
	r3.setContext("echo.sleepMS", "-400");
	c1.addRequest("system.echo.0.raw", r3);

	c1.executeAndWaitSent();
	int i1 = c1.executeAndWaitAny();
	System.out.println("E&WAny returns " + i1);
	if ((i1 != 1001) && (i1 != 1002) && (i1 != 1003)) {
	    System.out.println("E&W should only return 1001, 1002 or 1003 not " + i1);
	}

	if (!r1.inProgress()) throw new Exception("r1 should be inProgress");
	c1.executeAndWaitOne(r2);
	if (r2.inProgress()) throw new Exception("r2 should have finished");

	int wany = c1.executeAndWaitAny();
	System.out.println("E&WAny returns " + wany);
	if (wany == 0) throw new Exception("E&WAny returned zero");

	c1.executeAndWaitAll();
	System.out.println("four");
	String resp = r1.getResponseData();
	System.out.println("First response=" + resp);

	byte[] resp2 = r2.getResponseDataBytes();
	String s3 = new String(resp2);
	System.out.println("Second response=" + s3);

	System.out.println("Service Name: " + r1.getServiceName());
	if (r1.getServiceName().length() == 0) throw new Exception("Service name length must be > 0");

	if (r1.hasFault()) throw new Exception("r1 should not have a fault");
	if (!r3.hasFault()) throw new Exception("r3 should have fault");
	System.out.println("r3 faultcode=" + r3.getFaultCode());
	System.out.println("r3 faultText1=" + r3.getFaultText());
	System.out.println("r3 faultText2=" + r3.getFaultText());
	String se = new String(r3.getFaultText());
	String sw = "sleep time negative";
	System.out.println("r3 faultText3=" + se);
	System.out.println("r3 faultTextW=" + sw);
	if (r3.getFaultCode() != 111) throw new Exception("Expected 111 faultCode");
	if (!se.equals(sw)) throw new Exception("Expected '" + sw + "' fault text");

	// Check that reset, resets

	com.yahoo.pluton.ClientRequest r4 = new com.yahoo.pluton.ClientRequest();
	r4.setClientHandle(1004);
	r4.reset();
	if (r4.getClientHandle() != 0) throw new Exception("Client handle should be zero");
    }
}
