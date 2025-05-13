#if !defined FTPServer_H
#define FTPServer_H

#include <list>
#include "ClientConnection.h"

int define_socket_TCP(int port) ;

class FTPServer {
public:
   FTPServer(int port = 0); // A port with value 0 means that any free port will be chosen

   void run();

   void stop();

private:
   int port = -1;
   int msock = -1;
   std::list<ClientConnection *> connection_list;
};

#endif