#if !defined ClientConnection_H
#define ClientConnection_H

#include <pthread.h>
#include <cstdio>
#include <cstdint>

const int MAX_BUFF = 1000;

class ClientConnection {
public:
   ClientConnection(int s);

   ~ClientConnection();

   void WaitForRequests();

   void stop();

private:
   bool ok;  // This variable is a flag that avoids that the server listens if initialization errors occurred.
   FILE *control_fd;    // C file descriptor. We use it to buffer the  control connection of the socket,
                        // and it allows to manage it as a C file using fprintf, fscanf, etc.
   char command[MAX_BUFF];  // Buffer for saving the command.
   char arg[MAX_BUFF];      // Buffer for saving the arguments.
   int data_socket = -1;         // Data socket descriptor
   int control_socket;      // Control socket descriptor
   bool stop_server;
};

#endif
