#include <cstring>
#include <cstdio>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include "../lib/ClientConnection.h"

#include "../lib/common.h"
#include "../lib/FTPServer.h"
#include <sstream>
#include <fcntl.h>

// Variables globales para modo pasivo
static int PASSIVE_MODE = 0;
static int PASSIVE_SOCKET = -1;

// Constructor de la conexión con el cliente
ClientConnection::ClientConnection(int s) {
   control_socket = s;

   // Abrir el descriptor de archivo como flujo
   control_fd = fdopen(s, "a+");
   if (control_fd == nullptr) {
      std::cout << "Connection closed" << std::endl;
      fclose(control_fd);
      close(control_socket);
      ok = false;
      return;
   }
   ok = true;
   data_socket = -1;
   stop_server = false;
};

// Destructor: cerrar los sockets y el flujo
ClientConnection::~ClientConnection() {
   fclose(control_fd);
   close(control_socket);
}

// Conexión TCP activa a una dirección y puerto dados
int connect_TCP(uint32_t address, uint16_t port) {
   struct sockaddr_in sin{};
   int s;

   s = socket(AF_INET, SOCK_STREAM, 0);
   if (s < 0) errexit("No puedo crear el socket: %s\n", strerror(errno));

   sin.sin_family = AF_INET;
   sin.sin_addr.s_addr = address;
   sin.sin_port = htons(port);

   if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
      errexit("No puedo hacer el bind con el puerto: %s\n", strerror(errno));
   }
   return s;
}

// Crear un socket TCP en modo pasivo y obtener puerto/IP
int passive_socket_TCP(int control_socket, int *port, uint32_t *ip) {
  struct sockaddr_in sin{};
  int s;

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) errexit("No puedo crear el socket en pasivo: %s\n", strerror(errno));

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(0);

  if(bind(s, (sockaddr *)&sin, sizeof(sin)) < 0)
   errexit("No puedo hacer el bind en pasivo: %s\n", strerror(errno));
  
  socklen_t len = sizeof(sin);
  getsockname(s, (sockaddr *)&sin, &len);

  *port = ntohs(sin.sin_port);
  sockaddr_in sin_control{};
  socklen_t len_control = sizeof(sin_control);
  getsockname(control_socket, (sockaddr *)&sin_control, &len_control);

  *ip = ntohl(sin_control.sin_addr.s_addr);

  if(listen(s, 1) < 0)
   errexit("No puedo hacer el listen en pasivo: %s\n", strerror(errno));

  return s;
}

// Detener la conexión con el cliente
void ClientConnection::stop() {
   close(data_socket);
   close(control_socket);
   stop_server = true;
}

// Macro para comparar comandos
#define COMMAND(cmd) strcmp(command, cmd)==0

// Bucle principal para atender las peticiones del cliente
void ClientConnection::WaitForRequests() {
   uint32_t ip = -1;
   uint16_t port = -1;
   if (!ok) {
      return;
   }
   fprintf(control_fd, "220 Service ready\n");
   while (!stop_server) {
      fscanf(control_fd, "%s", command);
      // Comando USER: nombre de usuario
      if (COMMAND("USER")) {
         fscanf(control_fd, "%s", arg);
         fprintf(control_fd, "331 User name ok, need password\n");
      } 
      // Comando PWD: no implementado
      else if (COMMAND("PWD")) { 
      } 
      // Comando PASS: contraseña
      else if (COMMAND("PASS")) {
         fscanf(control_fd, "%s", arg);
         if (strcmp(arg, "1234") == 0) {
            fprintf(control_fd, "230 User logged in\n");
         } else {
            fprintf(control_fd, "530 Not logged in.\n");
            stop_server = true;
            break;
         }
      } 
      // Comando PORT: modo activo, conectar al cliente
      else if (COMMAND("PORT")) {
         fscanf(control_fd, "%s", arg); // Leer argumento
         int h1, h2, h3, h4, p1, p2;
         if (sscanf(arg, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2) == 6) {
             uint32_t ip = htonl((h1 << 24) | (h2 << 16) | (h3 << 8) | h4);
             uint16_t port = (p1 << 8) | p2;
             data_socket = connect_TCP(ip, port); // Conectar al cliente
             if (data_socket < 0) {
                 fprintf(control_fd, "425 Can't open data connection.\n");
             } else {
                 fprintf(control_fd, "200 OK\n");
             }
         } else {
            fprintf(control_fd, "501 Syntax error in parameters or arguments.\n");
            continue;
         }
      } 
      // Comando PASV: modo pasivo, el servidor espera conexión del cliente
      else if (COMMAND("PASV")) {
         int port;
         uint32_t ip;
         PASSIVE_SOCKET = passive_socket_TCP(control_socket, &port, &ip);
         PASSIVE_MODE = 1;

         int h1 = (ip >> 24) & 0xFF, 
             h2 = (ip >> 16) & 0xFF,
             h3 = (ip >> 8) & 0xFF,
             h4 = ip & 0xFF;
         int p1 = port / 256,
             p2 = port % 256;

         fprintf(control_fd, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\n", h1, h2, h3, h4, p1, p2);
         fflush(control_fd);
      } 
      // Comando STOR: subir archivo al servidor
      else if (COMMAND("STOR")) {
        fscanf(control_fd, "%s", arg);
        FILE *fp = fopen(arg, "wb");
        if(!fp) {
         fprintf(control_fd, "550 Cannot create file.\n");
         fflush(control_fd);
         continue;
        }

        if(!PASSIVE_MODE) {
         fprintf(control_fd, "150 File creation ok\n");
        } else {
         fprintf(control_fd, "150 File creation ok, about to open data connection\n");
        }
        fflush(control_fd);

        int data = PASSIVE_MODE ? accept(PASSIVE_SOCKET, nullptr, nullptr) : data_socket;
        char buffer[8192];
        ssize_t bytes;
        // Recibir datos y escribir en el archivo
        while((bytes = recv(data, buffer, sizeof(buffer), 0)) > 0 ) {
          fwrite(buffer, 1, bytes, fp);
        }
        fclose(fp);

        // Cerrar sockets según el modo
        if(PASSIVE_MODE) {
         close(data);
         close(PASSIVE_SOCKET);
         PASSIVE_SOCKET = -1;
         PASSIVE_MODE = 0;
        } else {
         close(data_socket);
         data_socket = -1;
        }

        fprintf(control_fd, "226 Closing data connection.\n");
        fflush(control_fd);
      } 
      // Comando RETR: descargar archivo del servidor
      else if (COMMAND("RETR")) {
        fscanf(control_fd, "%s", arg);
        FILE *fp = fopen(arg, "rb");
        if(!fp) {
         fprintf(control_fd, "550 Failed to open file\n");
         fflush(control_fd);
         continue;
        }

        fprintf(control_fd, "150 File status okay\n");
        fflush(control_fd);

        int data = PASSIVE_MODE ? accept(PASSIVE_SOCKET, nullptr, nullptr) : data_socket;
        char buffer[8192];
        ssize_t bytes;
        // Leer archivo y enviar al cliente
        while((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0 ) {
          send(data, buffer, bytes, 0);
        }
        fclose(fp);

        // Cerrar sockets según el modo
        if(PASSIVE_MODE) {
         close(data);
         close(PASSIVE_SOCKET);
         PASSIVE_SOCKET = -1;
         PASSIVE_MODE = 0;
        } else {
         close(data_socket);
         data_socket = -1;
        }

        fprintf(control_fd, "226 Closing data connection.\n");
        fflush(control_fd);
      } 
      // Comando LIST: listar archivos del directorio
      else if (COMMAND("LIST")) {

         FILE *fp = popen("ls", "r");
         if(!fp) {
            fprintf(control_fd, "550 Cannot open directory.\n");
            close(data_socket);
            data_socket = -1;
            continue;
         }

         if(!PASSIVE_MODE) {
            if(data_socket == -1) {
               fprintf(control_fd, "425 Can't open data connection.\n");
               continue;
            }

            fprintf(control_fd, "150 File status okay; about to open data connection.\n");
            fflush(control_fd);

            char buffer[1024];
            ssize_t bytes; 
            // Enviar listado por el socket de datos
            while((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
               send(data_socket, buffer, bytes, 0);
            }

            pclose(fp);
            close(data_socket);
            data_socket = -1;
         
            fprintf(control_fd, "250 List completed successfully.\n");
            fflush(control_fd);
         } else {
            fprintf(control_fd, "125 List started OK.\n");
            fflush(control_fd);
         
            int data = accept(PASSIVE_SOCKET, nullptr, nullptr);

            char buffer[1024];
            // Enviar listado por el socket de datos pasivo
            while (fgets(buffer, sizeof(buffer), fp)) {
               send(data, buffer, strlen(buffer), 0);
            }
            pclose(fp);
            close(data);

            close(PASSIVE_SOCKET);
            PASSIVE_SOCKET = -1;
            PASSIVE_MODE = 0;

            fprintf(control_fd, "250 List completed successfully.\n");
            fflush(control_fd);
         }
      } 
      // Comando SYST: tipo de sistema
      else if (COMMAND("SYST")) {
         fprintf(control_fd, "215 UNIX Type: L8.\n");
      } 
      // Comando TYPE: tipo de transferencia
      else if (COMMAND("TYPE")) {
         fscanf(control_fd, "%s", arg);
         fprintf(control_fd, "200 OK\n");
      } 
      // Comando QUIT: cerrar conexión
      else if (COMMAND("QUIT")) {
         fprintf(control_fd, "221 Service closing control connection. Logged out if appropriate.\n");
         close(data_socket);
         stop_server = true;
         break;
      } 
      // Comando FEAT y MDTM: no implementados
      else if (COMMAND("FEAT")) {
         fprintf(control_fd, "502 Command not implemented.\n");
         fflush(control_fd);
      } else if (COMMAND("MDTM")) {
         fprintf(control_fd, "502 Command not implemented.\n");
         fflush(control_fd);
      } 
      // Cualquier otro comando: no implementado
      else {
         fprintf(control_fd, "502 Command not implemented.\n");
         fflush(control_fd);
         printf("Comando : %s %s\n", command, arg);
         printf("Error interno del servidor\n");
      }
   }

   fclose(control_fd); // Cerrar el flujo de control al finalizar
};