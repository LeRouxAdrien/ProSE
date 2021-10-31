#include <stdio.h>  // printf
#include <stdlib.h> // NULL, malloc, EXIT_SUCCESS
#include <unistd.h>

#include "watchdog/watchdog.h"
#include "comBenno/postmanBenno/postmanBenno.h"
#include "tourManager/tourManager.h"
#include "./binScanner/adminScanner/adminScanner.h"
#include "./binScanner/scanner/scanner.h"
#include "./uiBenno/leds/leds.h"
#include "./uiBenno/speaker/speaker.h"


int main(int argc, char *argv[])
{

    printf("\033[33mLancement du programme \033[0m \n");

    //LANCEMENT DES MACHINES A ETATS DE POSTMAN ET TOUR MANAGER
    TourManager_startBenno();
    PostmanBenno_new();
    PostmanBenno_start();
    TourManager_new();
    TourManager_start();

    while (1)
    {
        
    }
}
