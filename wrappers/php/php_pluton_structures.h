#ifndef PHP_PLUTON_STRUCTS_H
#define PHP_PLUTON_STRUCTS_H

#include "pluton/client.h"
#include "pluton/clientRequest.h"
#include "pluton/service.h"

typedef struct _php_client_object {
  zend_object zo;
  pluton::client *C;
} php_client_object;

typedef struct _php_clientRequest_object {
  zend_object zo;
  pluton::clientRequest *R;
} php_clientRequest_object;

typedef struct _php_service_object {
  zend_object zo;
  pluton::service *S;
} php_service_object;


#endif
