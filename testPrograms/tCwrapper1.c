#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pluton/service_C.h>

/* Exercise the server-side C wrapper */

static void
fail(const char* msg)
{
  fprintf(stderr, "Failed: %s\n", msg);
  if (pluton_service_C_hasFault()) {
    fprintf(stderr, "Fault: %d - %s\n", pluton_service_C_getFaultCode(),
	    pluton_service_C_getFaultMessage("Prefix", 1));
  }

  exit(1);
}

int
main(int argc, char** argv)
{
  const char* ptr;
  const char* context;
  int len;
  char response[1000];

  if (!pluton_service_C_initialize()) fail("pluton_service_C_initialize");

  printf("Starting with API Version: %s\n", pluton_service_C_getAPIVersion());

  while (pluton_service_C_getRequest()) {
    pluton_service_C_getRequestData(&ptr, &len);

    switch (*ptr) {
    case 'F':
      pluton_service_C_sendFault(12, "You asked for a fault");
      break;

    case 'C':
      context = pluton_service_C_getContext("K12");
      if (!context) context = "No Context found for K12";
      sprintf(response, "Context for K12=%s\n", context);
      pluton_service_C_sendResponse(response, strlen(response));
      break;

    default:
      sprintf(response, "Request Length = %d\n SK=%s.%s.%d.%c and %s\nClient=%s\n",
	      len,
	      pluton_service_C_getServiceApplication(),
	      pluton_service_C_getServiceFunction(),
	      pluton_service_C_getServiceVersion(),
	      pluton_service_C_getSerializationType(),
	      pluton_service_C_getServiceKey(),
	      pluton_service_C_getClientName());
      pluton_service_C_sendResponse(response, strlen(response));
      break;
    }
  }

  if (pluton_service_C_hasFault()) {
    fprintf(stderr, "Got a fault %d and %s\n",
	    pluton_service_C_getFaultCode(),
	    pluton_service_C_getFaultMessage("My prefix", 1));
  }


  pluton_service_C_terminate();

  exit(0);
}
