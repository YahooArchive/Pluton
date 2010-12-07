#include <jni.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pluton/client_C.h>

#include "com_yahoo_pluton_ClientRequest.h"
#include "com_yahoo_pluton_Client.h"

#include "faultExceptions.c.txt"

/*
 * Convert return codes into exceptions.
 * (from http://twiki.corp.yahoo.com/view/Panama/SwigJavaBridging)
 */

static void
ThrowException(JNIEnv *jenv, const char *exceptionClass, const char *message) 
{
  jclass class;

  if (!jenv || !exceptionClass) return;

  (*jenv)->ExceptionClear(jenv);

  class = (*jenv)->FindClass(jenv, exceptionClass);
  if (!class) {
    fprintf(stderr, "Error, com_yahoo_pluton_clientJNI cannot find exception class: %s\n", exceptionClass);
    return;
  }

  (*jenv)->ThrowNew(jenv, class, message);
}

/* Return true if the faultCode is mapped to an exception */

static int
clientFaultToException(JNIEnv *jenv, pluton_client_C_obj* Cptr)
{
  int fCode;
  const char* fText;
  const char* exception;

  if (!pluton_client_C_hasFault(Cptr)) return 0;

  fCode = pluton_client_C_getFaultCode(Cptr);
  fText = pluton_client_C_getFaultMessage(Cptr, 0, 1);	/* Long format */
  exception = faultCodeToException(fCode);

  ThrowException(jenv, exception, fText);

  return 1;
}


static int
requestFaultToException(JNIEnv *jenv, pluton_request_C_obj* Rptr)
{
  int fCode;
  const char* fText;
  const char* exception;

  if (!pluton_request_C_hasFault(Rptr)) return 0;

  fCode = pluton_request_C_getFaultCode(Rptr);
  fText = pluton_request_C_getFaultText(Rptr);
  exception = faultCodeToException(fCode);

  ThrowException(jenv, exception, fText);

  return 0;
}


/*
 * Signature: ()J
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1new(JNIEnv* env, jclass this)
{
  return (jint) pluton_request_C_new();
}


/*
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1delete(JNIEnv* env, jclass this, jint Rptr)
{
  pluton_request_C_delete((pluton_request_C_obj*) Rptr);
}


/*
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1reset(JNIEnv* env, jclass this, jint Rptr)
{
  pluton_request_C_reset((pluton_request_C_obj*) Rptr);
}


/*
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1setRequestData(JNIEnv* env, jclass this, jint Rptr,
						 jstring requestData)
{
  const char* str;
  int len;

  str = (*env)->GetStringUTFChars(env, requestData, NULL); 
  if (!str) return;
  len = (*env)->GetStringUTFLength(env, requestData);

  /*
   * Ask the C interface to take a copy of the requestData so that
   * java string can be released now. For efficiency, we could avoid
   * the release until the request has been sent and then release the
   * jstring. That gets tricker though as we have to track the jstring
   * and worry about leaking them.
   */

  pluton_request_C_setRequestData((pluton_request_C_obj*) Rptr, str, len, 1);
  (*env)->ReleaseStringUTFChars(env, requestData, str);
}


/*
 * Signature: (I[B)V
 */
JNIEXPORT void JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1setRequestDataBytes(JNIEnv* env, jclass this, jint Rptr,
						      jbyteArray ar)
{
  jbyte* jba;
  int len;

  jba = (*env)->GetByteArrayElements(env, ar, NULL);
  len = (*env)->GetArrayLength(env, ar);
  pluton_request_C_setRequestData((pluton_request_C_obj*) Rptr, (const char*) jba, len, 1);
  (*env)->ReleaseByteArrayElements(env, ar, jba, JNI_ABORT);
}


/*
 * Signature: (II)V
 */
JNIEXPORT void JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1setAttribute(JNIEnv* env, jclass this, jint Rptr, jint attrs)
{
  pluton_request_C_setAttribute((pluton_request_C_obj*) Rptr, attrs);
}


/*
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1getAttribute(JNIEnv* env, jclass this, jint Rptr, jint attrs)
{
  return pluton_request_C_getAttribute((pluton_request_C_obj*) Rptr, attrs);
}


/*
 * Signature: (II)V
 */
JNIEXPORT void JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1clearAttribute(JNIEnv* env, jclass this, jint Rptr, jint attrs)
{
  pluton_request_C_clearAttribute((pluton_request_C_obj*) Rptr, attrs);
}


/*
 * Signature: (ILjava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1setContext(JNIEnv* env, jclass this, jint Rptr,
					     jstring k, jstring v)
{
  const char *key;
  const char *value;
  int ret;

  key = (*env)->GetStringUTFChars(env, k, NULL); 
  if (!key) return 0;
  value = (*env)->GetStringUTFChars(env, v, NULL);
  if (!value) {
    (*env)->ReleaseStringUTFChars(env, k, key);
    return 0;
  }

  ret = pluton_request_C_setContext((pluton_request_C_obj*) Rptr, key, value, strlen(value));
  if (!ret) requestFaultToException(env, (pluton_request_C_obj*) Rptr);

  (*env)->ReleaseStringUTFChars(env, k, key);
  (*env)->ReleaseStringUTFChars(env, v, value);

  return ret;
}


/*
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1getResponseLength(JNIEnv* env, jclass this, jint Rptr)
{
  const char* rp;
  int rl;
  int res;

 res = pluton_request_C_getResponseData((pluton_request_C_obj*) Rptr, &rp, &rl);
  if (res) {
    requestFaultToException(env, (pluton_request_C_obj*) Rptr);
    return 0;
  }

  return rl;
}


/*
 * Signature: (I)[B
 *
 * This interface differs from the C or C++ pluton interface in that the
 * return value is the responseData. The caller has to make a second
 * call to get the faultCode, if any.
 */
JNIEXPORT jbyteArray JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1getResponseDataBytes(JNIEnv* env, jclass this, jint Rptr)
{
  const char* rp;
  int rl;
  int res;
  jbyteArray ret;

  res = pluton_request_C_getResponseData((pluton_request_C_obj*) Rptr, &rp, &rl);
  if (res) {
    requestFaultToException(env, (pluton_request_C_obj*) Rptr);
    return (*env)->NewByteArray(env, 0);		/* Return an empty array */
  }

  /* Create and set the new array */

  ret = (*env)->NewByteArray(env, rl);
  (*env)->SetByteArrayRegion(env, ret, 0, rl, (jbyte*) rp);

  return ret;

#ifdef NO
  /* Convert to a C-String */

  rdMalloc = malloc(rl+1);
  if (!rdMalloc) return (*env)->NewStringUTF(env, "");
  strncpy(rdMalloc, rp, rl);
  rdMalloc[rl] = '\0';

  ret = (*env)->NewStringUTF(env, rdMalloc);

  free(rdMalloc);

  return ret;
#endif
}


/*
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1inProgress(JNIEnv* env, jclass this, jint Rptr)
{
  return pluton_request_C_inProgress((pluton_request_C_obj*) Rptr);
}


/*
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1hasFault(JNIEnv* env, jclass this, jint Rptr)
{
  return pluton_request_C_hasFault((pluton_request_C_obj*) Rptr);
}


/*
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1getFaultCode(JNIEnv* env, jclass this, jint Rptr)
{
  return pluton_request_C_getFaultCode((pluton_request_C_obj*) Rptr);
}


/*
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1getFaultText(JNIEnv* env, jclass this, jint Rptr)
{
  return (*env)->NewStringUTF(env, pluton_request_C_getFaultText((pluton_request_C_obj*) Rptr));
}


/*
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1getServiceName(JNIEnv* env, jclass this, jint Rptr)
{
  return (*env)->NewStringUTF(env, pluton_request_C_getServiceName((pluton_request_C_obj*) Rptr));
}


/*
 * Signature: (II)V
 */
JNIEXPORT void JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1setClientHandle(JNIEnv* env, jclass this,
						      jint Rptr, jint handle)
{
  pluton_request_C_setClientHandle((pluton_request_C_obj*) Rptr, (void*) handle);
}


/*
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_ClientRequest_JNI_1getClientHandle(JNIEnv* env, jclass this, jint Rptr)
{
  return (jint) pluton_request_C_getClientHandle((pluton_request_C_obj*) Rptr);
}


/*
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Client_JNI_1new(JNIEnv* env, jclass this, jstring who, jint toMS)
{
  const char *str;
  jint ret;

  str = (*env)->GetStringUTFChars(env, who, NULL);
  if (!str) return 0;	/* Wrapper throws an exception */

  ret = (jint) pluton_client_C_new(str, toMS);

  (*env)->ReleaseStringUTFChars(env, who, str);

  return ret;
}


/*
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_com_yahoo_pluton_Client_JNI_1delete(JNIEnv* env, jclass this, jint Cptr)
{
  pluton_client_C_delete((pluton_client_C_obj*) Cptr);
}


/*
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_com_yahoo_pluton_Client_JNI_1reset(JNIEnv* env, jclass this, jint Cptr)
{
  pluton_client_C_reset((pluton_client_C_obj*) Cptr);
}


/*
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_yahoo_pluton_Client_JNI_1getAPIVersion(JNIEnv* env, jclass this)
{
  return (*env)->NewStringUTF(env, pluton_client_C_getAPIVersion());
}


/*
 * Signature: (ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Client_JNI_1initialize(JNIEnv* env, jclass this,
						jint Cptr, jstring lookupMap)
{
  const char* str;
  int ret;

  str = (*env)->GetStringUTFChars(env, lookupMap, NULL);
  if (!str) return 0;

  ret = pluton_client_C_initialize((pluton_client_C_obj*) Cptr, str);
  if (ret) clientFaultToException(env, (pluton_client_C_obj*) Cptr);

  (*env)->ReleaseStringUTFChars(env, lookupMap, str);

  return ret;
}


/*
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Client_JNI_1hasFault(JNIEnv* env, jclass this, jint Cptr)
{
  return pluton_client_C_hasFault((pluton_client_C_obj*) Cptr);
}


/*
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Client_JNI_1getFaultCode(JNIEnv* env, jclass this, jint Cptr)
{
  return pluton_client_C_getFaultCode((pluton_client_C_obj*) Cptr);
}


/*
 * Signature: (ILjava/lang/String;I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_yahoo_pluton_Client_JNI_1getFaultMessage(JNIEnv* env, jclass this, jint Cptr,
						     jstring prefix, int longFormat)
{
  const char *str;
  const char* rp;

  str = (*env)->GetStringUTFChars(env, prefix, NULL); 
  if (!str) {
    ThrowException(env, "com/yahoo/pluton/client", "prefix string failed UTF conversion");
    return (*env)->NewStringUTF(env, "");
  }

  rp = pluton_client_C_getFaultMessage((pluton_client_C_obj*) Cptr, str, longFormat);

  (*env)->ReleaseStringUTFChars(env, prefix, str);

  return (*env)->NewStringUTF(env, rp);
}


/*
 * Signature: (II)V
 */
JNIEXPORT void JNICALL
Java_com_yahoo_pluton_Client_JNI_1setDebug(JNIEnv* env, jclass this, jint Cptr, jint nv)
{
  pluton_client_C_setDebug((pluton_client_C_obj*) Cptr, nv);
}


/*
 * Signature: (II)V
 */
JNIEXPORT void JNICALL
Java_com_yahoo_pluton_Client_JNI_1setTimeoutMilliSeconds(JNIEnv* env, jclass this,
							    jint Cptr, jint ms)
{
  pluton_client_C_setTimeoutMilliSeconds((pluton_client_C_obj*) Cptr, ms);
}


/*
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Client_JNI_1getTimeoutMilliSeconds(JNIEnv* env, jclass this, jint Cptr)
{
  return pluton_client_C_getTimeoutMilliSeconds((pluton_client_C_obj*) Cptr);
}


/*
 * Signature: (ILjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Client_JNI_1addRequest(JNIEnv* env, jclass this,
						jint Cptr, jstring serviceKey, jint Rptr)
{
  /**********************************************************************
   * Construct a C-string of the Service Key. This string only needs
   * to exists across the call to addRequest as the impl makes a copy
   * of the SK.
   **********************************************************************/

  int ret;
  const char *str;

  str = (*env)->GetStringUTFChars(env, serviceKey, NULL);
  if (!str) {
    ThrowException(env, "com/yahoo/pluton/client", "serviceKey failed UTF conversion");
    return -31;
  }

  ret = pluton_client_C_addRequest((pluton_client_C_obj*) Cptr, str, (pluton_request_C_obj*) Rptr);
  if (ret) clientFaultToException(env, (pluton_client_C_obj*) Cptr);

  (*env)->ReleaseStringUTFChars(env, serviceKey, str);

  return ret;
}


/*
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Client_JNI_1executeAndWaitSent(JNIEnv* env, jclass this, jint Cptr)
{
  int ret;

  ret = pluton_client_C_executeAndWaitSent((pluton_client_C_obj*) Cptr);
  if (ret <= 0) clientFaultToException(env, (pluton_client_C_obj*) Cptr);

  return ret;
}


/*
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Client_JNI_1executeAndWaitAll(JNIEnv* env, jclass this, jint Cptr)
{
  int ret;

  ret = pluton_client_C_executeAndWaitAll((pluton_client_C_obj*) Cptr);
  if (ret <= 0) clientFaultToException(env, (pluton_client_C_obj*) Cptr);

  return ret;
}


/*
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Client_JNI_1executeAndWaitOne(JNIEnv* env, jclass this,
						       jint Cptr, jint Rptr)
{
  int ret;

  ret = pluton_client_C_executeAndWaitOne((pluton_client_C_obj*) Cptr,
					  (pluton_request_C_obj*) Rptr);
  if (ret <= 0) clientFaultToException(env, (pluton_client_C_obj*) Cptr);

  return ret;
}


/*
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Client_JNI_1executeAndWaitAny(JNIEnv* env, jclass this, jint Cptr)
{
  pluton_request_C_obj* rPtr;

  rPtr = (pluton_request_C_obj*) pluton_client_C_executeAndWaitAny((pluton_client_C_obj*) Cptr);
  if (!rPtr) {
    clientFaultToException(env, (pluton_client_C_obj*) Cptr);
    return 0;
  }

  return (jint) pluton_request_C_getClientHandle(rPtr);
}
