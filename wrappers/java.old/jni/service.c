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

#include <jni.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pluton/service_C.h>

extern void pluton_service_C_insideLinuxJVM();

#include "com_yahoo_pluton_Service.h"

static void
ThrowException(JNIEnv *jenv, const char *exceptionClass, const char *message) 
{
  jclass class;

  if (!jenv || !exceptionClass) return;

  (*jenv)->ExceptionClear(jenv);

  class = (*jenv)->FindClass(jenv, exceptionClass);
  if (!class) {
    fprintf(stderr, "Error, com_yahoo_pluton_service cannot find exception class: %s\n", exceptionClass);
    return;
  }

  (*jenv)->ThrowNew(jenv, class, message);
}


/*
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Service_JNI_1initialize(JNIEnv* env, jclass this)
{
  pluton_service_C_insideLinuxJVM();
  return pluton_service_C_initialize();
}


/*
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_yahoo_pluton_Service_JNI_1getAPIVersion(JNIEnv* env, jclass this)
{
  return (*env)->NewStringUTF(env, pluton_service_C_getAPIVersion());
}


/*
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_com_yahoo_pluton_Service_JNI_1terminate(JNIEnv* env, jclass this)
{
  pluton_service_C_terminate();
}


/*
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Service_JNI_1getRequest(JNIEnv* env, jclass this)
{
  return pluton_service_C_getRequest();
}


/*
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Service_JNI_1sendResponse(JNIEnv* env, jclass this, jstring responseData)
{
  const char* str;
  int len;
  int ret;

  str = (*env)->GetStringUTFChars(env, responseData, NULL); 
  if (!str) return 0;
  len = (*env)->GetStringUTFLength(env, responseData);

  ret = pluton_service_C_sendResponse(str, len);

  (*env)->ReleaseStringUTFChars(env, responseData, str);

  return ret;
}


/*
 * Signature: ([B)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Service_JNI_1sendResponseBytes(JNIEnv* env, jclass this, jbyteArray ar)
{
  jbyte* jba;
  int len;
  int ret;

  jba = (*env)->GetByteArrayElements(env, ar, NULL);
  len = (*env)->GetArrayLength(env, ar);
  ret = pluton_service_C_sendResponse((const char*) jba, len);
  (*env)->ReleaseByteArrayElements(env, ar, jba, JNI_ABORT);

  return ret;
}


/*
 * Signature: (ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Service_JNI_1sendFault(JNIEnv* env, jclass this, jint fCode, jstring faultData)
{
  const char* str;

  str = (*env)->GetStringUTFChars(env, faultData, NULL); 
  if (!str) return 0;

  return pluton_service_C_sendFault(fCode, str);
}


/*
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Service_JNI_1hasFault(JNIEnv* env, jclass this)
{
  return pluton_service_C_hasFault();
}


/*
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Service_JNI_1getFaultCode(JNIEnv* env, jclass this)
{
  return pluton_service_C_getFaultCode();
}


/*
 * Signature: (Ljava/lang/String;I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_yahoo_pluton_Service_JNI_1getFaultMessage(JNIEnv* env, jclass this,
						 jstring prefix, int longFormat)
{
  const char *str;
  const char* rp;

  str = (*env)->GetStringUTFChars(env, prefix, NULL); 
  if (!str) {
    ThrowException(env, "com/yahoo/pluton/service", "prefix string failed UTF conversion");
    return (*env)->NewStringUTF(env, "");
  }

  rp = pluton_service_C_getFaultMessage(str, longFormat);

  (*env)->ReleaseStringUTFChars(env, prefix, str);

  return (*env)->NewStringUTF(env, rp);
}


/*
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_yahoo_pluton_Service_JNI_1getServiceKey(JNIEnv* env, jclass this)
{
  return (*env)->NewStringUTF(env, pluton_service_C_getServiceKey());
}


/*
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_yahoo_pluton_Service_JNI_1getServiceApplication(JNIEnv* env, jclass this)
{
  return (*env)->NewStringUTF(env, pluton_service_C_getServiceApplication());
}


/*
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_yahoo_pluton_Service_JNI_1getServiceFunction(JNIEnv* env, jclass this)
{
  return (*env)->NewStringUTF(env, pluton_service_C_getServiceFunction());
}


/*
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_com_yahoo_pluton_Service_JNI_1getServiceVersion(JNIEnv* env, jclass this)
{
  return pluton_service_C_getServiceVersion();
}


/*
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_yahoo_pluton_Service_JNI_1getSerializationType(JNIEnv* env, jclass this)
{
  const char* type = "unknownSerialization";
  switch (pluton_service_C_getSerializationType()) {
  case 'c': type = "COBOL"; break;
  case 'h': type = "HTML"; break;
  case 'm': type = "JMS"; break;
  case 'j': type = "JSON"; break;
  case 'n': type = "NETSTRING"; break;
  case 'p': type = "PHP"; break;
  case 's': type = "SOAP"; break;
  case 'x': type = "XML"; break;
  case 'r': type = "raw"; break;
  default: break;
  }

  return (*env)->NewStringUTF(env, type);
}


/*
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_yahoo_pluton_Service_JNI_1getClientName(JNIEnv* env, jclass this)
{
  return (*env)->NewStringUTF(env, pluton_service_C_getClientName());
}


/*
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_yahoo_pluton_Service_JNI_1getRequestDataBytes(JNIEnv* env, jclass this)
{
  const char* rp;
  int rl;
  jbyteArray ret;

  pluton_service_C_getRequestData(&rp, &rl);

  /* Create and set the new array */

  ret = (*env)->NewByteArray(env, rl);
  (*env)->SetByteArrayRegion(env, ret, 0, rl, (jbyte*) rp);

  return ret;
}


/*
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_yahoo_pluton_Service_JNI_1getContext(JNIEnv* env, jclass this, jstring key)
{
  const char* str;
  jstring ret;

  str = (*env)->GetStringUTFChars(env, key, NULL);
  if (!str) return (*env)->NewStringUTF(env, "");

  ret = (*env)->NewStringUTF(env, pluton_service_C_getContext(str));

  (*env)->ReleaseStringUTFChars(env, key, str);

  return ret;
}
