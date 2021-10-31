#include <stdio.h> // printf
#include <stdlib.h> // NULL, malloc, EXIT_SUCCESS
#include <unistd.h> // sleep

#include "server.h"

int main(int argc, char *argv[])
{
    Server_start();
    Server_readMsg();
    Server_stop();
    
    return EXIT_SUCCESS;
}
