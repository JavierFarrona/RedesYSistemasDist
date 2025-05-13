#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <list>
#include <iostream>
#include "../lib/common.h"
#include "../lib/FTPServer.h"
#include "../lib/ClientConnection.h"

// Crea un socket TCP, lo asocia a un puerto y lo pone en modo escucha
int define_socket_TCP(int port = 0) {
   struct sockaddr_in sin;
   int s;

   // Crear el socket TCP
   s = socket(AF_INET, SOCK_STREAM, 0);
   if (s < 0) errexit("No puedo crear el socket: %s\n", strerror(errno));

   // Inicializar la estructura de dirección
   memset(&sin, 0, sizeof(sin));
   sin.sin_family = AF_INET;
   sin.sin_addr.s_addr = INADDR_ANY;
   sin.sin_port = htons(port);

   // Asociar el socket a la dirección y puerto
   if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
     errexit("No puedo hacer el bind: %s\n", strerror(errno));

   // Obtener el puerto asignado (si era 0)
   socklen_t len = sizeof(sin);
   getsockname(s, (struct sockaddr *)&sin, &len);
   port = ntohs(sin.sin_port);

   // Mostrar en qué puerto está escuchando
   fprintf(stdout, "Listening in 127.0.0.1 %d\n", port);
   fflush(stdout);

   // Poner el socket en modo escucha
   if (listen(s, 5) < 0)
     errexit("Fallo en el listen: %s\n", strerror(errno));

   return s;
}

// Función que ejecuta la conexión con el cliente en un hilo
void *run_client_connection(void *c) {
   ClientConnection *connection = (ClientConnection *) c;
   connection->WaitForRequests();
   return nullptr;
}

// Constructor del servidor FTP, almacena el puerto
FTPServer::FTPServer(int port) {
   this->port = port;
}

// Detiene el servidor cerrando el socket principal
void FTPServer::stop() {
   close(msock);
   shutdown(msock, SHUT_RDWR);
}

// Arranca el servidor: acepta conexiones y crea un hilo por cliente
void FTPServer::run() {
   struct sockaddr_in fsin;
   int ssock;
   socklen_t alen = sizeof(fsin);
   msock = define_socket_TCP(port); // Crea y asocia el socket principal

   while (true) {
      pthread_t thread;
      // Espera una nueva conexión de cliente
      ssock = accept(msock, (struct sockaddr *) &fsin, &alen);
      if (ssock < 0)
         errexit("Fallo en el accept: %s\n", strerror(errno));
      // Crea un objeto para manejar la conexión con el cliente
      auto *connection = new ClientConnection(ssock);
      // Crea un hilo para atender al cliente de forma concurrente
      pthread_create(&thread, nullptr, run_client_connection, (void *) connection);
   }
}