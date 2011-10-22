/*
Copyright (c) 2010, Yahoo! Inc. All rights reserved.

Redistribution and use of this software in source and binary forms, with or
without modification, are permitted provided that the following conditions are
met: 

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation and/or 
other materials provided with the distribution.

* Neither the name of Yahoo! Inc. nor the names of its contributors may be used 
to endorse or promote products derived from this software without specific prior 
written permission of Yahoo! Inc.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

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
