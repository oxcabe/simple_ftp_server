//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//
//                     2ª de grado de Ingeniería Informática
//
//  Clase que atiende una petición FTP.
//
//****************************************************************************



#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <iostream>
#include <dirent.h>

#include "../include/common.h"

#include "../include/ClientConnection.h"


ClientConnection::ClientConnection(int s) {
    int sock = (int)(s);

    char buffer[MAX_BUFF];

    control_socket = s;
    // Consultar la documentación para conocer el funcionamiento de fdopen.
    fd = fdopen(s, "a+");
    if (fd == NULL){
	std::cout << "Connection closed" << std::endl;

	fclose(fd);
	close(control_socket);
	ok = false;
	return ;
    }

    ok = true;
    data_socket = -1;
};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket);

}


int connect_TCP( uint32_t address, uint16_t  port) {
    struct sockaddr_in sin;
    int s;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = address;
    sin.sin_port = htons(port);


    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0) {
        errexit("No se pudo crear el socket: %s\n", strerror(errno));
    }

    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
       errexit("No se ha podido conectar con %s: %s\n", inet_ntoa(sin.sin_addr), strerror(errno));
    }

    return s;

}

void ClientConnection::stop() {
    close(data_socket);
    close(control_socket);
    parar = true;

}

#define COMMAND(cmd) !strcmp(command, cmd)

// Función que atiende peticiones
void ClientConnection::WaitForRequests() {
    if (!ok) {
	 return;
    }

    fprintf(fd, "220 Service ready\n");
    while(!parar) {
      std::cout << "Waiting for instructions...\n";
      fscanf(fd, "%s", command);

      std::cout << "User issued the following command: " << command << '\n';

      if      (COMMAND("USER")) { user(); }
      else if (COMMAND("PWD"))  { pwd();  }
      else if (COMMAND("PASS")) { pass(); }
      else if (COMMAND("PORT")) { port(); }
      else if (COMMAND("PASV")) { pasv(); }
      else if (COMMAND("CWD"))  { cwd();  }
      else if (COMMAND("STOR")) { stor(); }
      else if (COMMAND("TYPE")) { type(); }
      else if (COMMAND("RETR")) { retr(); }
      else if (COMMAND("LIST")) { list(); }
      else if (COMMAND("SYST")) { syst(); }
      else if (COMMAND("QUIT")) { quit(); }
      else                      { defc(); }
    }

    fclose(fd);

    return;
};

void ClientConnection::user() {
    fscanf(fd, "%s", arg);

    if (std::strcmp(arg, "user")) {
        fprintf(fd, "332 Need account for login.\n");
        stop();
    }

    fprintf(fd, "331 User name ok, need password\n");
}

void ClientConnection::pwd() {
    char server_path[MAX_BUFF];

    if (getcwd(server_path, MAX_BUFF)) {
        fprintf(fd, "257 %s \n", server_path);
    }
}

void ClientConnection::pass() {
    fscanf(fd, "%s", arg);

    if (std::strcmp(arg, "user")) {
        fprintf(fd, "530 Not logged in.\n");
        stop();
    }

    fprintf(fd, "230 User logged in!\n");
}

void ClientConnection::port() {
    passive = false;

    unsigned addr_b[4], port_b[2];

    fscanf(fd, "%d,%d,%d,%d,%d,%d", &addr_b[0], &addr_b[1], &addr_b[2],
        &addr_b[3], &port_b[0], &port_b[1]);

    unsigned addr = addr_b[3]<<24 | addr_b[2]<<16 |
        addr_b[1]<<8 | addr_b[0];
    unsigned port = port_b[0] << 8 | port_b[1];

    data_socket = connect_TCP(addr,port);

    fprintf(fd, "200 OK\n");
}

void ClientConnection::pasv() {
    passive = true;

    struct sockaddr_in sin, sock_addr;
    socklen_t sock_addr_len = sizeof(sock_addr);
    int s = socket(AF_INET, SOCK_STREAM, 0);

    // Only works if client connects from localhost
    int sv_addr = 16777343;


    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = sv_addr;
    sin.sin_port = 0;

    if (s < 0) {
        errexit("No se pudo crear el socket: %s\n", strerror(errno));
    }

    if (bind(s, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        errexit("No se ha podido hacer bind con el puerto: %s\n",
          strerror(errno));
    }

    if (listen(s, 5) < 0) {
        errexit("Fallo en listen: %s\n", strerror(errno));
    }

    getsockname(s, (struct sockaddr *)&sock_addr, &sock_addr_len);

    fprintf(fd, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).\n",
        (unsigned int)(sv_addr & 0xff),
        (unsigned int)((sv_addr >> 8) & 0xff),
        (unsigned int)((sv_addr >> 16) & 0xff),
        (unsigned int)((sv_addr >> 24) & 0xff),
        (unsigned int)(sock_addr.sin_port & 0xff),
        (unsigned int)(sock_addr.sin_port >> 8));

    data_socket = s;
}

void ClientConnection::cwd() {
    char aux[200];

	   getcwd(aux, sizeof(aux));

	   if(!chdir(aux)) {
         fprintf(fd, "250 Working directory changed.");
       } else if (chdir(aux) < 0) {
           fprintf(fd, "431 No such directory.");
       }
}

void ClientConnection::stor() {
    fscanf(fd, "%s", arg);

    FILE* file = fopen(arg,"wb");

    if (!file) {
        fprintf(fd, "450 Requested file action not taken. File unavailable.\n");
        close(data_socket);
    } else {
        fprintf(fd, "150 File status okay; opening data connection.\n");
        fflush(fd);

        struct sockaddr_in socket_addr;
        socklen_t socket_addr_len = sizeof(socket_addr);
        char buffer[MAX_BUFF];
        int n;

        if (passive) {
            data_socket = accept(data_socket,(struct sockaddr *)&socket_addr,
                &socket_addr_len);
        }

        do {
            n = recv(data_socket, buffer, MAX_BUFF, 0);
            fwrite(buffer, sizeof(char) , n, file);
        } while (n == MAX_BUFF);

        fprintf(fd,"226 Closing data connection. Operation successfully completed.\n");
        fclose(file);
        close(data_socket);
   }
}

void ClientConnection::type() {
    fprintf(fd, "200 OK ASCII ONLY.\n");
}

void ClientConnection::retr() {
    fscanf(fd, "%s", arg);

    FILE* file = fopen(arg, "rb");

    if (!file) {
        fprintf(fd, "450 Requested file action not taken. File unavailable.\n");
        close(data_socket);
    } else {
        fprintf(fd, "150 File status okay - About to open data connection.\n");

        struct sockaddr_in socket_addr;
        socklen_t socket_addr_len = sizeof(socket_addr);
        char buffer[MAX_BUFF];
        int n;

        if (passive) {
            data_socket = accept(data_socket,(struct sockaddr *)&socket_addr,
              &socket_addr_len);
        }
        do {
            n = fread(buffer, sizeof(char), MAX_BUFF, file);
            send(data_socket, buffer, n, 0);

        } while (n == MAX_BUFF);

        fprintf(fd,"226 Closing data connection. Requested file action successful.\n");
        fclose(file);
        close(data_socket);
   }
}

void ClientConnection::list() {
    fprintf(fd, "125 Data connection already open; transfer starting\n");

    struct sockaddr_in socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    char buffer[MAX_BUFF];
    std::string ls_content = "";
    std::string ls = "ls -l";

    ls.append(" 2>&1");

    FILE* file = popen(ls.c_str(), "r");

    if (!file) {
        fprintf(fd, "450 Requested file action not taken. File unavailable.\n");
        close(data_socket);
    }

    else {

         if (passive) {
             data_socket = accept(data_socket,(struct sockaddr *)&socket_addr, &socket_addr_len);
         }

        while (!feof(file)) {
            if (fgets(buffer, MAX_BUFF, file) != NULL) {
                ls_content.append(buffer);
            }
        }

        send(data_socket, ls_content.c_str(), ls_content.size(), 0);

        fprintf(fd, "250 Closing data connection. Requested file action successful.\n");
        pclose(file);
        close(data_socket);
    }
}

void ClientConnection::syst() {
    fprintf(fd, "215 UNIX Type: L8\n");
}

void ClientConnection::quit() {
    fprintf(fd, "221 Logging out and closing connection\n");
    stop();
}

void ClientConnection::defc() {
    fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
    printf("Command: %s %s\n", command, arg);
    printf("Server internal error\n");
}
