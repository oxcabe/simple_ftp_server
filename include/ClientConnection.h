#if !defined ClientConnection_H
#define ClientConnection_H

#include <pthread.h>

#include <cstdio>
#include <cstdint>

const int MAX_BUFF=1000;

class ClientConnection {
public:
    ClientConnection(int s);
    ~ClientConnection();

    void WaitForRequests();
    void stop();


private:
   bool ok; // 	Esta variable es un flag que evita que el servidor
	    // escuche peticiones si ha habido errores en la inicialización.

    FILE *fd;	 // Descriptor de fichero en C. Se utiliza para que
		 // socket de la conexión de control tenga esté bufferado
		 // y se pueda manipular como un fichero a la C, con
		 // fprintf, fscanf, etc.

    char command[MAX_BUFF];  // Buffer para almacenar el comando.
    char arg[MAX_BUFF];      // Buffer para almacenar los argumentos.

    int data_socket;         // Descriptor de socket para la conexion de datos;
    int control_socket;      // Descriptor de socket para al conexión de control;

    bool passive;   // Modo pasivo
    bool parar;     // Fin de ejecucion de comandos

    // Implemented FTP commands
    void user();    // Login prompt
    void pwd();     // pwd
    void pass();    // Password prompt
    void port();    // Data port
    void pasv();    // Passive mode (pass)
    void cwd();     // cd
    void stor();    // put
    void type();    // Representation type
    void retr();    // get
    void list();    // ls
    void syst();    // system
    void quit();    // quit / bye
    void defc();    // Default case (Command not implemented)

};

#endif
