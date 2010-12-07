package com.yahoo.pluton;

public class Service {

    private final static native int JNI_initialize();
    private final static native String JNI_getAPIVersion();
    private final static native void JNI_terminate();

    private final static native int JNI_getRequest();
    private final static native int JNI_sendResponse(String responseData);
    private final static native int JNI_sendResponseBytes(byte[] responseData);
    private final static native int JNI_sendFault(int faultCode, String faultText);
    private final static native int JNI_hasFault();
    private final static native int JNI_getFaultCode();
    private final static native String JNI_getFaultMessage(String prefix, int longFormat);

    // Request access methods

    private final static native String JNI_getServiceKey();
    private final static native String JNI_getServiceApplication();
    private final static native String JNI_getServiceFunction();
    private final static native int JNI_getServiceVersion();
    private final static native String JNI_getSerializationType();
    private final static native String JNI_getClientName();

    private final static native byte[] JNI_getRequestDataBytes();
    private final static native String JNI_getContext(String key);


    public Service() {}
    public Service(String name) {}

    public boolean initialize() throws Exception
    {
	int ret = JNI_initialize();
	if (ret == 0) throw new Exception(JNI_getFaultMessage("", 1));

	return (ret != 0) ? true : false;
    }

    public static String getAPIVersion()
    {
	return JNI_getAPIVersion();
    }

    public void terminate()
    {
	JNI_terminate();
    }

    public boolean getRequest()
    {
	return (JNI_getRequest() != 0) ? true : false;
    }

    public boolean sendResponse(String responseData) throws Exception
    {
	int ret = JNI_sendResponse(responseData);
	if (ret == 0) throw new Exception(JNI_getFaultMessage("", 1));

	return (ret != 0) ? true : false;
    }

    public boolean sendResponseBytes(byte[] responseData) throws Exception
    {
	int ret = JNI_sendResponseBytes(responseData);

	if (ret == 0) throw new Exception(JNI_getFaultMessage("", 1));

	return (ret != 0) ? true : false;
    }

    public boolean sendFault(int faultCode, String faultText) throws Exception
    {
	int ret = JNI_sendFault(faultCode, faultText);
	if (ret == 0) throw new Exception(JNI_getFaultMessage("", 1));

	return (ret != 0) ? true : false;
    }

    public boolean hasFault()
    {
	return (JNI_hasFault() != 0) ? true : false;
    }

    public int getFaultCode()
    {
	return JNI_getFaultCode();
    }

    public String getFaultMessage(String prefix, boolean longFormat)
    {
	return JNI_getFaultMessage(prefix, longFormat ? 1 : 0);
    }

    // Request access methods

    public String getServiceKey()
    {
	return JNI_getServiceKey();
    }

    public String getServiceApplication()
    {
	return JNI_getServiceApplication();
    }

    public String getServiceFunction()
    {
	return JNI_getServiceFunction();
    }

    public int getServiceVersion()
    {
	return JNI_getServiceVersion();
    }

    public String getSerializationType()
    {
	return JNI_getSerializationType();
    }

    public String getClientName()
    {
	return JNI_getClientName();
    }


    public byte[] getRequestDataBytes()
    {
	return JNI_getRequestDataBytes();
    }


    public String getRequestData()
    {
	return new String(JNI_getRequestDataBytes());
    }


    public String getContext(String key)
    {
	return JNI_getContext(key);
    }

    static {
        try
            {
                System.loadLibrary("plutonjni");
            }
        catch (UnsatisfiedLinkError error)
            {
                System.err.println("Native load of 'plutonjni' failed for com.yahoo.pluton.Service");
		System.err.println("Check for the existence of /usr/local/lib/libplutonjni.so et al");
		System.err.println("Alternatively make sure you have -Djava.library.path set");
                error.printStackTrace();
                System.exit(1);
            }
    }
}
