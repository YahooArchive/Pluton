#ifndef _LISTENINTERFACE_H
#define _LISTENINTERFACE_H

#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

class listenInterface {
 public:
  listenInterface(const char* defaultInterface="", int defaultPort=0);
  ~listenInterface();

  const char* openAndListen(const char* interfaceAndPort, int queueDepth=20);
  int	transferFD();		// transfer ownership of fd to caller

 private:
  const char* resolve(const char* interface, int port);

  std::string		_interface;
  int			_port;
  struct addrinfo* 	_addrInfo;
  int			_fd;
};

#endif
