//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//
//                     2º de grado de Ingeniería Informática
//
//                    This class processes an FTP transactions.
//
//                            Javier Farrona Cabrera
//
//****************************************************************************

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <netdb.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <grp.h>
#include <langinfo.h>
#include <locale.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <dirent.h>
#include <iostream>
#include <sys/stat.h>

#include "common.h"

#include "ClientConnection.h"
#include "FTPServer.h"

ClientConnection::ClientConnection(int s)
{
  int sock = (int)(s);

  char buffer[MAX_BUFF];

  control_socket = s;
  // Check the Linux man pages to know what fdopen does.
  fd = fdopen(s, "a+");
  if (fd == NULL)
  {
    std::cout << "Connection closed" << std::endl;

    fclose(fd);
    close(control_socket);
    ok = false;
    return;
  }

  ok = true;
  data_socket = -1;
};

ClientConnection::~ClientConnection()
{
  fclose(fd);
  close(control_socket);
}

int connect_TCP(uint32_t address, uint16_t port)
{

  // ------ Implement your code to define a socket here --------------

  struct sockaddr_in sin; 
  int s;

  s = socket(AF_INET, SOCK_STREAM, 0); 

  if (s < 0)
  {
    errexit("No puedo crear el socket: %s\n", strerror(errno));
  }

  memset(&sin, 0, sizeof(sin)); 
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = address;
  sin.sin_port = htons(port);

  if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    errexit("No puedo hacer el bind con el puerto: %s\n", strerror(errno));
  }

  return s; // You must return the socket descriptor.
}

void ClientConnection::stop()
{
  close(data_socket);
  close(control_socket);
  parar = true;
}

#define COMMAND(cmd) strcmp(command, cmd) == 0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You
// are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests()
{
  if (!ok)
  {
    return;
  }

  fprintf(fd, "220 Service ready\n");

  while (!parar)
  {
    fscanf(fd, "%s", command);
    if (COMMAND("USER"))
    {
      fscanf(fd, "%s", arg);
      fprintf(fd, "331 User name ok, need password\n");
    }
    else if (COMMAND("PWD"))
    {
    }
    else if (COMMAND("PASS"))
    {
      fscanf(fd, "%s", arg);
      if (strcmp(arg, "1234") == 0)
      {
        fprintf(fd, "230 User logged in\n");
      }
      else
      {
        fprintf(fd, "530 Not logged in.\n");
        parar = true;
      }
    }
    else if (COMMAND("PWD"))
    { 
      fprintf(fd, "257 \"%s\" created", get_current_dir_name());
    }

    else if (COMMAND("PASS"))
    { 
      fscanf(fd, "%s", arg);
      int ContraCorrecta = strcmp(arg, "1234");

      if (ContraCorrecta == 0)
        fprintf(fd, "230 User logged in, proceed.\n");
      else
        fprintf(fd, "530 Not logged in.\n");
    }

    else if (COMMAND("PORT"))
    { 
      int h0, h1, h2, h3;
      int p0, p1;
      
      fscanf(fd, "%d, %d, %d, %d, %d, %d", &h0, &h1, &h2, &h3, &p0, &p1);
      uint32_t address = h3 << 24 | h2 << 16 | h1 << 8 | h0;
      uint16_t port = p0 << 8 | p1;

      data_socket = connect_TCP(address, port);
      fprintf(fd, "200. OK\n");
    }

    else if (COMMAND("PASV"))
    { 
      int s;
      int p0, p1;
      struct sockaddr_in sin;
      socklen_t slen = sizeof(sin);

      s = define_socket_TCP(0);

      getsockname(s, reinterpret_cast<sockaddr *>(&sin), &slen);
      uint16_t port = sin.sin_port;
      p0 = port >> 8;
      p1 = port & 0xFF; // port & 11111111
      fprintf(fd, "227 Entering Passive Mode (127,0,0,1,%d,%d).\n", p1, p0);
      fflush(fd);
      data_socket = accept(s, reinterpret_cast<sockaddr *>(&sin), &slen);
    }

    else if (COMMAND("CWD"))
    { 
      char *directorio;
      fscanf(fd, "%s", directorio);
      int resultado = chdir(directorio);
      if (resultado == 0)
        fprintf(fd, "CWD con exito\n");
      else
        fprintf(fd, "500 Syntax error, command unrecognized.\n");
    }

    else if (COMMAND("STOR"))
    { 
      fscanf(fd, "%s", arg);
      FILE *fp;
      fp = fopen(arg, "wb");
      if (fp == NULL)
      {
        fprintf(fd, "553 Requested action not taken.File name not allowed.\n");
        fprintf(fd, "226 Closing data connection.\n");
        close(data_socket);
      }
      else
      {

        fprintf(fd, "150 File status oka y; about to open data connection\n");
        fflush(fd);
        const int TAM_BUF = 2048;
        char buffer[TAM_BUF];
        while (1)
        {
          int b = recv(data_socket, buffer, TAM_BUF, 0);
          fwrite(buffer, 1, b, fp);
          if (b < TAM_BUF)
          {
            break;
          }
        }

        fprintf(fd, "226 Closing data connection.\n");
        fflush(fd);
        close(data_socket);
        fclose(fp);
      }
    }

    else if (COMMAND("SYST"))
    { 
      fprintf(fd, "215 UNIX Type: L8.\n");
    }

    else if (COMMAND("TYPE"))
    { 
      fscanf(fd, "%s", arg);
      fprintf(fd, "200 OK.\n");
    }

    else if (COMMAND("RETR"))
    { 
      fscanf(fd, "%s", arg);
      FILE *fp;
      fp = fopen(arg, "rb"); // wb
      if (fp == NULL)
      {
        fprintf(fd, "553 Requested action not taken.File name not allowed.\n");
        fprintf(fd, "226 Closing data connection.\n");
        close(data_socket);
      }
      else
      {

        fprintf(fd, "150 File status oka y; about to open data connection\n");
        const int TAM_BUF = 2048;
        char buffer[TAM_BUF];
        while (1)
        {
          int b = fread(buffer, 1, TAM_BUF, fp); // recv
          send(data_socket, buffer, b, 0);       // fwrite
          if (b < TAM_BUF)
          {
            break;
          }
        }

        fprintf(fd, "226 Closing data connection.\n");
        close(data_socket);
        fclose(fp);
      }
    }

    else if (COMMAND("QUIT"))
    { 
      fprintf(fd, "221 Service closing control connection..\n");
      stop();
    }

    else if (COMMAND("LIST"))
    { 
      DIR *dirp;
      dirp = opendir(get_current_dir_name());
      struct dirent *dp;
      fprintf(fd, "125 List started OK.\n");
      while ((dp = readdir(dirp)) != NULL)
      {
        std::string n = std::string(dp->d_name) + "\n";
        send(data_socket, n.c_str(), n.size(), 0);
      }
      closedir(dirp);
      fprintf(fd, "250 List completed successfully.\n");
      close(data_socket);
    }

    else
    {
      fprintf(fd, "502 Command not implemented.\n");
      fflush(fd);
      printf("Comando : %s %s\n", command, arg);
      printf("Error interno del servidor\n");
    }
  }

  fclose(fd);

  return;
};
