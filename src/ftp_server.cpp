#include <iostream>
#include <signal.h>

#include "../include/FTPServer.h"

FTPServer *server;

extern "C" void sighandler(int signal, siginfo_t *info, void *ptr) {
  std::cout << "Hasta pronto" << std::endl;
  server->stop();
  exit(-1);
}


void exit_handler() {
    server->stop();
}


int main(int argc, char **argv) {



    struct sigaction action;
    action.sa_sigaction = sighandler;
    action.sa_flags = SA_SIGINFO;
      sigaction(SIGINT, &action , NULL);
    server = new FTPServer(2121);

    std::cout << "FTP server v1.0\n";
    std::cout << "USER: user\nPASS: user\n";

      atexit(exit_handler);
    server->run();

}
