#include <iostream>
#include <csignal>

#include "../lib/common.h"
#include "../lib/FTPServer.h"

// Puntero global al servidor FTP
FTPServer *server;

// Manejador de señal para SIGINT (Ctrl+C)
// Detiene el servidor y termina el programa
extern "C" void sighandler(int signal, siginfo_t *info, void *ptr) {
   std::cout << "Received sigaction" << std::endl;
   server->stop();
   exit(-1);
}

// Función que se ejecuta al salir del programa (por atexit)
// Detiene el servidor limpiamente
void exit_handler() {
   server->stop();
}

// Imprime cómo usar el programa
void print_usage() {
   std::cout << "Usage: ftp_server [port]" << std::endl;
}

int main(int argc, char **argv) {
   // Configura el manejador de señal para SIGINT usando sigaction
   struct sigaction action{};
   action.sa_sigaction = sighandler;
   action.sa_flags = SA_SIGINFO;
   sigaction(SIGINT, &action, nullptr);

   // Si no se pasa puerto por argumento, usar cualquier puerto disponible
   if (argc == 1) {
      server = new FTPServer(0);
   } else {
      int port;
      // Si el argumento no es un entero, mostrar ayuda y salir
      if (sscanf(argv[1], "%i", &port) != 1) {
         print_usage();
         errexit("\tport must be an integer\n");
      }
      server = new FTPServer(port);
   }
   // Registrar función para detener el servidor al salir
   atexit(exit_handler);
   // Ejecutar el servidor (bucle principal)
   server->run();
}