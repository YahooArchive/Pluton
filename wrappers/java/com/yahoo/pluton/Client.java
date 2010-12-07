package com.yahoo.pluton;

import com.yahoo.pluton.ClientRequest;

public class Client {

    // Define the private JNI interfaces for this facade

    synchronized static native void JNI_initThreads();

    static native int JNI_new(String cPtr, int jarg2);
    static native void JNI_delete(int cPtr);
    static native void JNI_reset(int cPtr);

    static native String JNI_getAPIVersion();
    static native int JNI_initialize(int cPtr, String jarg2);
    static native int JNI_hasFault(int cPtr);
    static native int JNI_getFaultCode(int cPtr);
    static native String JNI_getFaultMessage(int cPtr, String prefix, int jarg3);

    static native void JNI_setDebug(int cPtr, int jarg2);
    static native void JNI_setTimeoutMilliSeconds(int cPtr, int jarg2);
    static native int JNI_getTimeoutMilliSeconds(int cPtr);

    static native int JNI_addRequest(int cPtr, String serviceKey, int rPtr);
    static native int JNI_executeAndWaitSent(int cPtr);
    static native int JNI_executeAndWaitAll(int cPtr);
    static native int JNI_executeAndWaitOne(int cPtr, int rPtr);
    static native int JNI_executeAndWaitAny(int cPtr);


    //////////////////////////////////////////////////
    // and now for the facade
    //////////////////////////////////////////////////

    public Client()
    {
	_cPtr = JNI_new("JNI", 0);
	if (_cPtr == 0) throw new OutOfMemoryError("client_new returned NULL");
    }

    public Client(String clientName, int defaultTimeoutMS)
    {
	_cPtr = JNI_new(clientName, defaultTimeoutMS);
	if (_cPtr == 0) throw new OutOfMemoryError("client_new returned NULL");
    }

    protected void finalize() throws Throwable {
	try {
	    if (_cPtr != 0) JNI_delete(_cPtr);
	} finally {
	    super.finalize();
	}
    }

    public void reset()
    {
	JNI_reset(_cPtr);
    }

    public static String getAPIVersion()
    {
	return JNI_getAPIVersion();
    }

    public int initialize(String lookupMap) throws IllegalArgumentException
    {
	int res = JNI_initialize(_cPtr, lookupMap);
	if (res == 0) throw new IllegalArgumentException(JNI_getFaultMessage(_cPtr, "", 1));

	return res;
    }

    public boolean hasFault()
    {
	return (JNI_hasFault(_cPtr) != 0) ? true : false;
    }
    public int getFaultCode() { return JNI_getFaultCode(_cPtr); }

    public String getFaultMessage(String prefix, boolean longFormatFlag)
    {
	return JNI_getFaultMessage(_cPtr, prefix, (longFormatFlag) ? 1: 0);
    }

    public void setDebug(boolean onOrOff)
    {
	JNI_setDebug(_cPtr, (onOrOff) ? 1 : 0);
    }

    public void setTimeoutMilliSeconds(int milliSeconds)
    {
	JNI_setTimeoutMilliSeconds(_cPtr, milliSeconds);
    }

    public int getTimeoutMilliSeconds()
    {
	return JNI_getTimeoutMilliSeconds(_cPtr);
    }

    public int addRequest(String serviceKey, com.yahoo.pluton.ClientRequest request)
    {
	int res = JNI_addRequest(_cPtr, serviceKey, request.getRPtr());
	if (res == 0) throw new IllegalArgumentException(JNI_getFaultMessage(_cPtr, "", 1));

	return res;
    }

    public int executeAndWaitSent() throws Error
    {
	int res = JNI_executeAndWaitSent(_cPtr);
	if (res < 0) throw new Error(JNI_getFaultMessage(_cPtr, "", 1));

	return res;
    }

    public int executeAndWaitAll() throws Error
    {
	int res = JNI_executeAndWaitAll(_cPtr);
	if (res < 0) throw new Error(JNI_getFaultMessage(_cPtr, "", 1));

	return res;
    }

    public int executeAndWaitOne(com.yahoo.pluton.ClientRequest request) throws Error
    {
	int res = JNI_executeAndWaitOne(_cPtr, request.getRPtr());
	if (res < 0) throw new Error(JNI_getFaultMessage(_cPtr, "", 1));

	return res;
    }

    public int executeAndWaitAny() throws Error
    {
	int res = JNI_executeAndWaitAny(_cPtr);
	if (JNI_hasFault(_cPtr) != 0) throw new Error(JNI_getFaultMessage(_cPtr, "", 1));

	return res;
    }


    ///

    private int _cPtr;

    ///

    static {
        try
            {
                System.loadLibrary("plutonjni");
                JNI_initThreads();       // Only ever call this once
            }
        catch (UnsatisfiedLinkError error)
            {
                System.err.println("Native load of 'plutonjni' failed for com.yahoo.pluton.Client");
		System.err.println("Check for the existence of /usr/local/lib/libplutonjni.so et al");
		System.err.println("Alternatively make sure you have -Djava.library.path set");
                error.printStackTrace();
                System.exit(1);
            }
    }
}
