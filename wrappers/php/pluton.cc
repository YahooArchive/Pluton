extern "C" {

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_pluton.h"
}

#include <string>

#include "php_pluton_structures.h"


//////////////////////////////////////////////////////////////////////
// We need to hold onto any request data that the php caller provides,
// so we usurp the clientHandle and use it as a pointer to our own
// structure which in turn houses the client handle.
//////////////////////////////////////////////////////////////////////

class myRequestData {
public:
  myRequestData() : callersHandle(0) {}
  long		callersHandle;		// In php, the callers handle is an long
  std::string	requestData;
};


// May need this later

PHP_INI_BEGIN()
PHP_INI_END()

// Handlers

static zend_object_handlers client_object_handlers;
static zend_object_handlers clientRequest_object_handlers;
static zend_object_handlers service_object_handlers;

// Class entries

zend_class_entry *client_entry;
zend_class_entry *clientRequest_entry;
zend_class_entry *service_entry;


//////////////////////////////////////////////////////////////////////
// client: constructor, new and free
//////////////////////////////////////////////////////////////////////

PHP_METHOD(PlutonClient, __construct)
{
  const char* yourName;
  int yourNameLen;
  unsigned int defaultTimeoutMilliSeconds = 4000;

  php_client_object *client_obj;
  zval *object = getThis();
  client_obj = (php_client_object *)zend_object_store_get_object(object TSRMLS_CC);

  if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, 
			   "s|l",
			   &yourName, &yourNameLen, 
			   &defaultTimeoutMilliSeconds) == FAILURE) {
    yourName = "phpWrapper";
  }

  client_obj->C = new pluton::client(yourName);
}


static void
php_client_object_free(void *object TSRMLS_DC)
{
  php_client_object *intern = (php_client_object *)object;
  if (!intern) return;

  if (intern->C) delete(intern->C);

  zend_object_std_dtor(&intern->zo TSRMLS_CC);
  efree(intern);
}


static zend_object_value
php_client_object_new(zend_class_entry *class_type TSRMLS_DC) {
  zval *tmp;
  zend_object_value retval;
  php_client_object *intern;

  intern = (php_client_object*) emalloc(sizeof(php_client_object));
  memset((void*) intern, 0, sizeof(php_client_object));

  zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
  zend_hash_copy(intern->zo.properties, &class_type->default_properties,
		 (copy_ctor_func_t) zval_add_ref,(void *) &tmp, sizeof(zval *));

  retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_client_object_free, NULL TSRMLS_CC);
  retval.handlers = (zend_object_handlers *) &client_object_handlers;

  return retval;
}


//////////////////////////////////////////////////////////////////////
// clientRequest: constructor, new and free
//////////////////////////////////////////////////////////////////////

PHP_METHOD(PlutonClientRequest, __construct)
{
  php_clientRequest_object *clientRequest_obj;
  zval *object = getThis();
  clientRequest_obj = (php_clientRequest_object *)zend_object_store_get_object(object TSRMLS_CC);

  clientRequest_obj->R = new pluton::clientRequest();

  clientRequest_obj->R->setClientHandle((void*) new myRequestData());

  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }
}


static void
php_clientRequest_object_free(void *object TSRMLS_DC)
{
  php_clientRequest_object *intern = (php_clientRequest_object *)object;
  if (!intern) return;

  if (intern->R) {
    myRequestData* mrd = (myRequestData*) intern->R->getClientHandle();
    delete(mrd);
    delete(intern->R);
  }

  zend_object_std_dtor(&intern->zo TSRMLS_CC);
  efree(intern);
}


static zend_object_value
php_clientRequest_object_new(zend_class_entry *class_type TSRMLS_DC) {
  zval *tmp;
  zend_object_value retval;
  php_clientRequest_object *intern;

  intern = (php_clientRequest_object*) emalloc(sizeof(php_clientRequest_object));
  memset((void*) intern, 0, sizeof(php_clientRequest_object));

  zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
  zend_hash_copy(intern->zo.properties, &class_type->default_properties,
		 (copy_ctor_func_t) zval_add_ref,(void *) &tmp, sizeof(zval *));

  retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_clientRequest_object_free, NULL TSRMLS_CC);
  retval.handlers = (zend_object_handlers *) &clientRequest_object_handlers;

  return retval;
}


//////////////////////////////////////////////////////////////////////
// service: constructor, new and free
//////////////////////////////////////////////////////////////////////

PHP_METHOD(PlutonService, __construct)
{
  char* serviceName = NULL;
  int serviceNameLen = 0;

  if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, 
			   "s",
			   &serviceName, &serviceNameLen) == FAILURE) {
    RETURN_NULL();
  }

  php_service_object *service_obj;
  zval *object = getThis();
  service_obj = (php_service_object *)zend_object_store_get_object(object TSRMLS_CC);

  service_obj->S = new pluton::service();
}


static void
php_service_object_free(void *object TSRMLS_DC)
{
  php_service_object *intern = (php_service_object *)object;
  if (!intern) return;

  if (intern->S) delete(intern->S);

  zend_object_std_dtor(&intern->zo TSRMLS_CC);
  efree(intern);
}


static zend_object_value
php_service_object_new(zend_class_entry *class_type TSRMLS_DC) {
  zval *tmp;
  zend_object_value retval;
  php_service_object *intern;

  intern = (php_service_object*) emalloc(sizeof(php_service_object));
  memset((void*) intern, 0, sizeof(php_service_object));

  zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
  zend_hash_copy(intern->zo.properties, &class_type->default_properties,
		 (copy_ctor_func_t) zval_add_ref,(void *) &tmp, sizeof(zval *));

  retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_service_object_free, NULL TSRMLS_CC);
  retval.handlers = (zend_object_handlers *) &service_object_handlers;

  return retval;
}


//////////////////////////////////////////////////////////////////////
// Get the pluton::client from the zval. Typical usage:
// pluton::client* C = php_to_client(getThis());
//////////////////////////////////////////////////////////////////////

static pluton::client*
php_to_client(zval* object)
{
  php_client_object* co = (php_client_object*) zend_object_store_get_object(object TSRMLS_CC);

  return co->C;
}


static pluton::clientRequest*
php_to_clientRequest(zval* object)
{
  php_clientRequest_object* cro =
    (php_clientRequest_object*) zend_object_store_get_object(object TSRMLS_CC);

  return cro->R;
}

static pluton::service*
php_to_service(zval* object)
{
  php_service_object* so =
    (php_service_object*) zend_object_store_get_object(object TSRMLS_CC);

  return so->S;
}


//////////////////////////////////////////////////////////////////////
// Start of standard client methods
//////////////////////////////////////////////////////////////////////

ZEND_METHOD(PlutonClient, getAPIVersion)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::client* C = php_to_client(getThis());

  RETURN_STRING(const_cast<char *>(C->getAPIVersion()), 1);
}


ZEND_METHOD(PlutonClient, initialize)
{
  char* lookupMapPath = "";
  int lookupMapPathLen = 0;

  if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, 
			   "|s",
			   &lookupMapPath, &lookupMapPathLen) == FAILURE) {
    RETURN_BOOL(0);
  }

  pluton::client* C = php_to_client(getThis());

  RETURN_BOOL(C->initialize(lookupMapPath));
}


ZEND_METHOD(PlutonClient, hasFault)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::client* C = php_to_client(getThis());

  RETURN_BOOL(C->hasFault());
}


ZEND_METHOD(PlutonClient, getFault)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::client* C = php_to_client(getThis());

  const pluton::fault& F = C->getFault();

  RETURN_STRING(const_cast<char*>(F.getMessage().c_str()), 1);
}



ZEND_METHOD(PlutonClient, getFaultCode)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::client* C = php_to_client(getThis());

  const pluton::fault& F = C->getFault();

  RETURN_LONG(F.getFaultCode());
}


ZEND_METHOD(PlutonClient, getFaultMessage)
{
  char* prefix = NULL;
  int prefixLen = 0;
  bool longFormat = true;

  if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, 
			   "sb",
			   &prefix, &prefixLen, &longFormat) == FAILURE) {
    RETURN_NULL();
  }

  pluton::client* C = php_to_client(getThis());

  const pluton::fault& F = C->getFault();

  std::string s = F.getMessage(prefix, longFormat);

  // This is wrong as 's' goes out of scope, unless the RETURN_STRING
  // macro makes a copy...

  RETURN_STRING(const_cast<char *>(s.c_str()), 1);
}


ZEND_METHOD(PlutonClient, setDebug)
{
  bool nv = false;
  if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, 
			   "b",
			   &nv) == FAILURE) {
    RETURN_BOOL(0);
  }

  pluton::client* C = php_to_client(getThis());
  C->setDebug(nv);
}


ZEND_METHOD(PlutonClient, setTimeoutMilliSeconds)
{
  unsigned int timeoutMilliSeconds = 0;
  if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, 
			   "l",
			   &timeoutMilliSeconds) == FAILURE) {
    RETURN_BOOL(0);
  }

  pluton::client* C = php_to_client(getThis());
  C->setTimeoutMilliSeconds(timeoutMilliSeconds);
}


ZEND_METHOD(PlutonClient, getTimeoutMilliSeconds)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::client* C = php_to_client(getThis());

  RETURN_LONG(C->getTimeoutMilliSeconds());
}


ZEND_METHOD(PlutonClient, addRequest)
{
  char* serviceKey = "";
  int serviceKeyLen = 0;
  zval *clientRequestObj;

  if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, 
			   "sO",
			   &serviceKey, &serviceKeyLen,
			   &clientRequestObj, clientRequest_entry) == FAILURE) {
    RETURN_BOOL(0);
  }

  pluton::clientRequest* R = php_to_clientRequest(clientRequestObj);
  pluton::client* C = php_to_client(getThis());

  RETURN_BOOL(C->addRequest(serviceKey, R));
}


ZEND_METHOD(PlutonClient, executeAndWaitSent)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::client* C = php_to_client(getThis());

  RETURN_BOOL(C->executeAndWaitSent());
}

ZEND_METHOD(PlutonClient, executeAndWaitAll)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::client* C = php_to_client(getThis());
	
  RETURN_LONG(C->executeAndWaitAll());
}

ZEND_METHOD(PlutonClient, executeAndWaitAny)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::client* C = php_to_client(getThis());
  pluton::clientRequest* R = C->executeAndWaitAny();
  if (R) {
    myRequestData* mrd = (myRequestData *) R->getClientHandle();
    RETURN_LONG(mrd->callersHandle);
  }

  RETURN_LONG(0);
}


ZEND_METHOD(PlutonClient, executeAndWaitOne)
{
  zval *clientRequestObj;
  if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, 
			   "O",
			   &clientRequestObj, clientRequest_entry) == FAILURE) {
    RETURN_BOOL(0);
  }

  pluton::clientRequest* R = php_to_clientRequest(clientRequestObj);
  pluton::client* C = php_to_client(getThis());

  RETURN_BOOL(C->executeAndWaitOne(R));
}


ZEND_METHOD(PlutonClient, reset)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::client* C = php_to_client(getThis());
	
  C->reset();
}


static zend_function_entry php_PlutonClient_methods[] = {
	PHP_ME(PlutonClient, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClient, addRequest, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClient, executeAndWaitAll, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClient, executeAndWaitAny, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClient, executeAndWaitOne, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClient, executeAndWaitSent, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClient, getAPIVersion, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClient, getFault, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClient, getFaultCode, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClient, getFaultMessage, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClient, getTimeoutMilliSeconds, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClient, hasFault, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClient, initialize, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClient, reset, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClient, setDebug, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClient, setTimeoutMilliSeconds, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};


//////////////////////////////////////////////////////////////////////
// clientRequest methods
//////////////////////////////////////////////////////////////////////

ZEND_METHOD(PlutonClientRequest, setRequestData)
{
  char* request = NULL;
  int requestLen = 0;

  if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, 
			   "s",
			   &request, &requestLen) == FAILURE) {
    RETURN_NULL();
  }


  ////////////////////////////////////////////////////////////
  // Transfer the request data to the std::string in our own
  // structure so that the data is secure across the "async"
  // calls.
  ////////////////////////////////////////////////////////////

  pluton::clientRequest* R = php_to_clientRequest(getThis());


  // No checks are made to determine whether the request is in
  // progress or not. The clientAPI probably should have returned a
  // fault in this case - instead the caller gets it when they call
  // addRequest.

  myRequestData* mrd = (myRequestData*) R->getClientHandle();
  mrd->requestData.assign(request, requestLen);

  // Give pluton the std::string reference as that will not change

  R->setRequestData(mrd->requestData.data(), mrd->requestData.length());

  RETURN_NULL();
}


ZEND_METHOD(PlutonClientRequest, getFaultCode)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::clientRequest* R = php_to_clientRequest(getThis());
  pluton::faultCode f = R->getFaultCode();

  RETURN_LONG(f);
}

ZEND_METHOD(PlutonClientRequest, getFaultText)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::clientRequest* R = php_to_clientRequest(getThis());

  std::string text = R->getFaultText();
  RETURN_STRING(const_cast<char *>(text.c_str()), 1);
}


ZEND_METHOD(PlutonClientRequest, getResponseData)
{
  zval *zresponseData;
  if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zresponseData) == FAILURE) {
    RETURN_NULL();
  }

  pluton::clientRequest* R = php_to_clientRequest(getThis());

  std::string responseData;
  pluton::faultCode f = R->getResponseData(responseData);

  convert_to_string(zresponseData);	  // make sure the variable is a string

  // enlarge buffer to hold the additional data

  Z_STRVAL_P(zresponseData) = (char *)erealloc(Z_STRVAL_P(zresponseData), responseData.length()+1);

  // Use c_str() to ensure a trailing \0 but copy for .length to get
  // any possible intermediate \0s

  memcpy(Z_STRVAL_P(zresponseData), responseData.c_str(), responseData.length()+1);
  Z_STRLEN_P(zresponseData) = responseData.length();	

  RETURN_LONG(f);
}


//////////////////////////////////////////////////////////////////////
// The "real" clientHandle is an instance of myRequestData so to store
// the callers clientHandle we store it in the myRequestData instance.
//
// The clientHandle from a php perspective is a long rather than a
// void*
//////////////////////////////////////////////////////////////////////

ZEND_METHOD(PlutonClientRequest, setClientHandle)
{
  long ch = 0;

  if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ch) == FAILURE) {
    RETURN_NULL();
  }

  pluton::clientRequest* R = php_to_clientRequest(getThis());

  myRequestData* mrd = (myRequestData*) R->getClientHandle();

  mrd->callersHandle = ch;
}


ZEND_METHOD(PlutonClientRequest, getClientHandle)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::clientRequest* R = php_to_clientRequest(getThis());

  myRequestData* mrd = (myRequestData*) R->getClientHandle();

  RETURN_LONG(mrd->callersHandle);
}


ZEND_METHOD(PlutonClientRequest, setContext)
{
  char* key = NULL;
  char* value = NULL;
  int keyLen = 0;
  int valueLen = 0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &key, &keyLen,
			    &value, &valueLen) == FAILURE) {
    RETURN_NULL();
  }

  pluton::clientRequest* R = php_to_clientRequest(getThis());

  std::string strValue(value);

  RETURN_BOOL(R->setContext(key, strValue));
}


ZEND_METHOD(PlutonClientRequest, getServiceName)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::clientRequest* R = php_to_clientRequest(getThis());

  std::string service = R->getServiceName();
  RETURN_STRING(const_cast<char *>(service.c_str()), 1);
}


ZEND_METHOD(PlutonClientRequest, inProgress)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }
  
  pluton::clientRequest* R = php_to_clientRequest(getThis());

  RETURN_BOOL(R->inProgress());
}


ZEND_METHOD(PlutonClientRequest, setAttribute)
{
  long int reqAttr=0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &reqAttr) == FAILURE) {
    return;
  }

  pluton::clientRequest* R = php_to_clientRequest(getThis());

  R->setAttribute(reqAttr);
}


ZEND_METHOD(PlutonClientRequest, clearAttribute)
{
  long int reqAttr=0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &reqAttr) == FAILURE) {
    return;
  }

  pluton::clientRequest* R = php_to_clientRequest(getThis());

  R->clearAttribute((int)reqAttr);
}


ZEND_METHOD(PlutonClientRequest, getAttribute)
{
  long int reqAttr=0;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &reqAttr) == FAILURE) {
    return;
  }

  pluton::clientRequest* R = php_to_clientRequest(getThis());

  RETURN_BOOL(R->getAttribute(reqAttr));
}


ZEND_METHOD(PlutonClientRequest, reset)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::clientRequest* R = php_to_clientRequest(getThis());

  // The clientHandle is used for internal purposes so it needs to be
  // saved across the reset and do the moral equivalent of the
  // reset on the emulated data.

  myRequestData* mrd = (myRequestData*) R->getClientHandle();
  R->reset();
  mrd->callersHandle = 0;
  mrd->requestData.erase();
  R->setClientHandle((void*) mrd);
}


ZEND_METHOD(PlutonClientRequest, hasFault)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::clientRequest* R = php_to_clientRequest(getThis());

  RETURN_BOOL(R->hasFault());
}


static zend_function_entry php_PlutonClientRequest_methods[] = {
	PHP_ME(PlutonClientRequest, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClientRequest, getClientHandle, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClientRequest, getFaultCode, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClientRequest, getFaultText, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClientRequest, getResponseData, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClientRequest, getServiceName, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClientRequest, inProgress, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClientRequest, setClientHandle, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClientRequest, setContext, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonClientRequest, setRequestData, NULL, ZEND_ACC_PUBLIC)
        PHP_ME(PlutonClientRequest, clearAttribute, NULL, ZEND_ACC_PUBLIC)
        PHP_ME(PlutonClientRequest, getAttribute, NULL, ZEND_ACC_PUBLIC)
        PHP_ME(PlutonClientRequest, hasFault, NULL, ZEND_ACC_PUBLIC)
        PHP_ME(PlutonClientRequest, reset, NULL, ZEND_ACC_PUBLIC)
        PHP_ME(PlutonClientRequest, setAttribute, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};


//////////////////////////////////////////////////////////////////////
// service methods
//////////////////////////////////////////////////////////////////////

ZEND_METHOD(PlutonService, initialize)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::service* S = php_to_service(getThis());

  RETURN_BOOL(S->initialize());
}


ZEND_METHOD(PlutonService, getAPIVersion)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::service* S = php_to_service(getThis());

  RETURN_STRING(const_cast<char *>(S->getAPIVersion()), 1);
}


ZEND_METHOD(PlutonService, terminate)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::service* S = php_to_service(getThis());
  S->terminate();
}


ZEND_METHOD(PlutonService, getRequest)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::service* S = php_to_service(getThis());

  bool ret = S->getRequest();

  RETURN_BOOL(ret);
}


ZEND_METHOD(PlutonService, sendResponse)
{
  char* response = NULL;
  int responseLen = 0;

  if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, 
			   "s",
			   &response, &responseLen) == FAILURE) {
    RETURN_NULL();
  }
	
  pluton::service* S = php_to_service(getThis());

  RETURN_BOOL(S->sendResponse(response, responseLen));
}


ZEND_METHOD(PlutonService, sendFault)
{
  int faultCode = 0;
  char* faultText = NULL;
  int faultTextLen = 0;

  pluton::service* S = php_to_service(getThis());

  if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, 
			   "ls",
			   &faultCode, 
			   &faultText, &faultTextLen) == FAILURE) {
    RETURN_NULL();
  }
	
  RETURN_BOOL(S->sendFault(faultCode, faultText));
}


ZEND_METHOD(PlutonService, getServiceKey)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  std::string serviceKey;
  pluton::service* S = php_to_service(getThis());
  S->getServiceKey(serviceKey);

  RETURN_STRING(const_cast<char *>(serviceKey.c_str()), 1);
}


ZEND_METHOD(PlutonService, getServiceApplication)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  std::string serviceApplication;
  pluton::service* S = php_to_service(getThis());
  S->getServiceApplication(serviceApplication);

  RETURN_STRING(const_cast<char *>(serviceApplication.c_str()), 1);
}


ZEND_METHOD(PlutonService, getServiceFunction)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  std::string serviceFunction;
  pluton::service* S = php_to_service(getThis());
  S->getServiceFunction(serviceFunction);

  RETURN_STRING(const_cast<char *>(serviceFunction.c_str()), 1);
}


ZEND_METHOD(PlutonService, getServiceVersion)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::service* S = php_to_service(getThis());

  RETURN_LONG(S->getServiceVersion());
}


ZEND_METHOD(PlutonService, getSerializationType)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::service* S = php_to_service(getThis());

  RETURN_LONG(S->getSerializationType());
}


ZEND_METHOD(PlutonService, getClientName)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  std::string clientName;
  pluton::service* S = php_to_service(getThis());
  S->getClientName(clientName);

  RETURN_STRING(const_cast<char *>(clientName.c_str()), 1);
}


ZEND_METHOD(PlutonService, getRequestData)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  std::string request;

  pluton::service* S = php_to_service(getThis());
  S->getRequestData(request);

  RETURN_STRING(const_cast<char *>(request.c_str()), 1);
}


ZEND_METHOD(PlutonService, getContext)
{
  char *key = NULL;
  int keyLen = 0;

  if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, 
			   "s", &key, &keyLen) == FAILURE) {
    RETURN_NULL();
  }

  std::string context;
  pluton::service* S = php_to_service(getThis());
  bool contextPresent = S->getContext(key, context);

  if(contextPresent) {
    RETURN_STRING(const_cast<char *>(context.c_str()), 1);
  }
  else {
    RETURN_NULL();
  }
}


ZEND_METHOD(PlutonService, hasFault)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::service* S = php_to_service(getThis());

  RETURN_BOOL(S->hasFault());
}


ZEND_METHOD(PlutonService, getFault)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::service* S = php_to_service(getThis());

  const pluton::fault& F = S->getFault();

  RETURN_STRING(const_cast<char*>(F.getMessage().c_str()), 1);
}



ZEND_METHOD(PlutonService, getFaultCode)
{
  if(ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  pluton::service* S = php_to_service(getThis());

  const pluton::fault& F = S->getFault();

  RETURN_LONG(F.getFaultCode());
}


ZEND_METHOD(PlutonService, getFaultMessage)
{
  char* prefix = NULL;
  int prefixLen = 0;
  bool longFormat = true;

  if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, 
			   "sb",
			   &prefix, &prefixLen, &longFormat) == FAILURE) {
    RETURN_NULL();
  }

  pluton::service* S = php_to_service(getThis());

  const pluton::fault& F = S->getFault();

  std::string s = F.getMessage(prefix, longFormat);

  // This is wrong as 's' goes out of scope, unless the RETURN_STRING
  // macro makes a copy...

  RETURN_STRING(const_cast<char *>(s.c_str()), 1);
}



static zend_function_entry php_PlutonService_methods[] = {
	PHP_ME(PlutonService, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, getAPIVersion, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, getClientName, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, getContext, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, getFault, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, getFaultCode, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, getFaultMessage, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, getRequest, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, getRequestData, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, getSerializationType, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, getServiceApplication, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, getServiceFunction, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, getServiceKey, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, getServiceVersion, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, hasFault, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, initialize, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, sendFault, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, sendResponse, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(PlutonService, terminate, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};


PHP_MINIT_FUNCTION(yahoo_pluton)
{
  zend_class_entry ce;

  memcpy(&client_object_handlers,
	 zend_get_std_object_handlers(), sizeof(zend_object_handlers));
  memcpy(&service_object_handlers,
	 zend_get_std_object_handlers(), sizeof(zend_object_handlers));
  memcpy(&clientRequest_object_handlers,
	 zend_get_std_object_handlers(), sizeof(zend_object_handlers));

  // Register PlutonClient Class

  INIT_CLASS_ENTRY(ce, "PlutonClient", php_PlutonClient_methods);
  ce.create_object = php_client_object_new;
  client_object_handlers.clone_obj = NULL;
  client_entry = zend_register_internal_class(&ce TSRMLS_CC);


  // Register PlutonClientRequest Class

  INIT_CLASS_ENTRY(ce, "PlutonClientRequest", php_PlutonClientRequest_methods);
  ce.create_object = php_clientRequest_object_new;
  clientRequest_object_handlers.clone_obj = NULL;
  clientRequest_entry = zend_register_internal_class(&ce TSRMLS_CC);


  // Register PlutonService Class

  INIT_CLASS_ENTRY(ce, "PlutonService", php_PlutonService_methods);
  ce.create_object = php_service_object_new;
  service_object_handlers.clone_obj = NULL;
  service_entry = zend_register_internal_class(&ce TSRMLS_CC);

  REGISTER_INI_ENTRIES();

  REGISTER_LONG_CONSTANT("PLUTON_SERIALIZATION_UNKNOWN",
			 pluton::unknownSerialization,
			 CONST_CS|CONST_PERSISTENT);
  REGISTER_LONG_CONSTANT("PLUTON_SERIALIZATION_COBOL",
			 pluton::COBOL,
			 CONST_CS|CONST_PERSISTENT);
  REGISTER_LONG_CONSTANT("PLUTON_SERIALIZATION_HTML",
			 pluton::HTML,
			 CONST_CS|CONST_PERSISTENT);
  REGISTER_LONG_CONSTANT("PLUTON_SERIALIZATION_JMS",
			 pluton::JMS,
			 CONST_CS|CONST_PERSISTENT);
  REGISTER_LONG_CONSTANT("PLUTON_SERIALIZATION_JSON",
			 pluton::JSON,
			 CONST_CS|CONST_PERSISTENT);
  REGISTER_LONG_CONSTANT("PLUTON_SERIALIZATION_NETSTRING",
			 pluton::NETSTRING,
			 CONST_CS|CONST_PERSISTENT);
  REGISTER_LONG_CONSTANT("PLUTON_SERIALIZATION_PHP",
			 pluton::PHP,
			 CONST_CS|CONST_PERSISTENT);
  REGISTER_LONG_CONSTANT("PLUTON_SERIALIZATION_SOAP",
			 pluton::SOAP,
			 CONST_CS|CONST_PERSISTENT);
  REGISTER_LONG_CONSTANT("PLUTON_SERIALIZATION_XML",
			 pluton::XML,
			 CONST_CS|CONST_PERSISTENT);
  REGISTER_LONG_CONSTANT("PLUTON_SERIALIZATION_RAW",
			 pluton::raw,
			 CONST_CS|CONST_PERSISTENT);

  // Register attributes constants

  REGISTER_LONG_CONSTANT("PLUTON_NOWAIT_ATTR",
			 pluton::noWaitAttr,
			 CONST_CS|CONST_PERSISTENT);

  REGISTER_LONG_CONSTANT("PLUTON_NOREMOTE_ATTR",
			 pluton::noRemoteAttr,
			 CONST_CS|CONST_PERSISTENT);

  REGISTER_LONG_CONSTANT("PLUTON_NORETRY_ATTR",
			 pluton::noRetryAttr,
			 CONST_CS|CONST_PERSISTENT);

  REGISTER_LONG_CONSTANT("PLUTON_KEEPAFFINITY_ATTR",
			 pluton::keepAffinityAttr,
			 CONST_CS|CONST_PERSISTENT);

  REGISTER_LONG_CONSTANT("PLUTON_NEEDAFFINITY_ATTR",
			 pluton::needAffinityAttr,
			 CONST_CS|CONST_PERSISTENT);

  REGISTER_LONG_CONSTANT("PLUTON_ALL_ATTR",
			 pluton::allAttrs,
			 CONST_CS|CONST_PERSISTENT);

  return SUCCESS;
}


PHP_MINFO_FUNCTION(yahoo_pluton)
{
  php_info_print_table_start();
  php_info_print_table_header( 2, "yahoo_pluton support", "enabled" );
  php_info_print_table_end();
}


zend_module_entry yahoo_pluton_module_entry = {
  STANDARD_MODULE_HEADER,
  "yahoo_pluton",
  NULL,				// Functions
  PHP_MINIT(yahoo_pluton),
  NULL,				// MSHUTDOWN
  NULL,				// RINIT
  NULL,				// RSHUTDOWN
  PHP_MINFO(yahoo_pluton),
  PHP_PLUTON_VERSION,
  STANDARD_MODULE_PROPERTIES
};


#ifdef COMPILE_DL_YAHOO_PLUTON
ZEND_GET_MODULE(yahoo_pluton)
#endif
