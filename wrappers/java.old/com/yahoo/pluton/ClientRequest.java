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

public class ClientRequest {


    //////////////////////////////////////////////////
    // Define the private JNI interfaces for this facade
    //////////////////////////////////////////////////

    static native int JNI_new();
    static native void JNI_delete(int rPtr);
    static native void JNI_reset(int rPtr);
    static native void JNI_setRequestData(int rPtr, String requestData);
    static native void JNI_setRequestDataBytes(int rPtr, byte[] requestData);

    static native void JNI_setAttribute(int rPtr, int jarg2);
    static native int JNI_getAttribute(int rPtr, int jarg2);
    static native void JNI_clearAttribute(int rPtr, int jarg2);

    static native int JNI_setContext(int rPtr, String jarg2, String jarg3);
    static native int JNI_getResponseLength(int rPtr);
    static native byte[] JNI_getResponseDataBytes(int rPtr);

    static native int JNI_inProgress(int rPtr);

    static native int JNI_hasFault(int rPtr);
    static native int JNI_getFaultCode(int rPtr);
    static native String JNI_getFaultText(int rPtr);
    static native String JNI_getServiceName(int rPtr);

    static native void JNI_setClientHandle(int rPtr, int jarg2);
    static native int JNI_getClientHandle(int rPtr);

    //////////////////////////////////////////////////
    // and now for the facde
    //////////////////////////////////////////////////

    public ClientRequest()
    {
	_rPtr = JNI_new();
	if (_rPtr == 0) throw new OutOfMemoryError("request_new returned NULL");
    }

    protected void finalize() throws Throwable {
	if (JNI_inProgress(_rPtr) != 0) {
	    throw new Exception("com.yahoo.pluton.clientRequest finalized but still inProgress");
	}
	try {
	    JNI_delete(_rPtr);
	} finally {
	    super.finalize();
	}
    }


    public void reset() { JNI_reset(_rPtr); }


    public void setRequestData(String requestData)
    {
	JNI_setRequestData(_rPtr, requestData);
    }


    public void setRequestDataBytes(byte[] b)
    {
	JNI_setRequestDataBytes(_rPtr, b);
    }

    public final static int noWaitAttr = 	0x0001;
    public final static int noRemoteAttr = 	0x0002;
    public final static int noRetryAttr = 	0x0004;
    public final static int keepAffinityAttr = 	0x0008;
    public final static int needAffinityAttr = 	0x0016;

    public void setAttribute(int attribute)
    {
	JNI_setAttribute(_rPtr, attribute);
    }
    public boolean getAttribute(int attribute)
    {
	return (JNI_getAttribute(_rPtr, attribute) != 0);
    }
    public void clearAttribute(int attribute)
    {
	JNI_clearAttribute(_rPtr, attribute);
    }


    public void setContext(String key, String value)
    {
	if (JNI_setContext(_rPtr, key, value) == 0) {
	    throw new IllegalArgumentException(JNI_getFaultText(_rPtr));
	}
    }


    public String getResponseData()
    {
	return new String(JNI_getResponseDataBytes(_rPtr));
    }


    public byte[] getResponseDataBytes()
    {
	return JNI_getResponseDataBytes(_rPtr);
    }


    public boolean inProgress() { return (JNI_inProgress(_rPtr) != 0); }

    public boolean hasFault() { return (JNI_hasFault(_rPtr) != 0); }
    public int getFaultCode() { return JNI_getFaultCode(_rPtr); }
    public String getFaultText() { return JNI_getFaultText(_rPtr); }
    public String getServiceName() { return JNI_getServiceName(_rPtr); }

    public void setClientHandle(int newHandle)
    {
	JNI_setClientHandle(_rPtr, newHandle);
    }
    public int getClientHandle()
    {
	return JNI_getClientHandle(_rPtr);
    }

    public int getRPtr() { return _rPtr; }

    ///

    private int _rPtr;
}
