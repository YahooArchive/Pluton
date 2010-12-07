import com.yahoo.pluton.Service;

class tJavaService1 {
    public static void main(String[] args) throws Throwable {
        System.out.println("Java Test Service Started");
	com.yahoo.pluton.Service s = new com.yahoo.pluton.Service("fault sender");
	s.initialize();

	while (s.getRequest()) {
	    String r = s.getRequestData();
	    String ctx = s.getContext("ctx");
	    System.out.println("ctx=" + ctx);
	    if (ctx.equals("sendfault")) {
		s.sendFault(123, r);
	    }
	    else {
		s.sendResponse("Expected a context of 'ctx'");
	    }
	}

	s.terminate();
    }
}
