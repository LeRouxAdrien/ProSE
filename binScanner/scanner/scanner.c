/**
 * @file scanner.c
 * @author Antoine Rivière
 * @brief scanner is a class to manage the start of scanning RFID tags, 
 * manage the end of a collect by closing all ports of the sensor 
 * and get the bin ID that is read.
 * @version 2.0
 * @date 2021-04-27
 * 
 * @copyright © 2021, Turfly company.
 * 
 */

#include <stdint.h>   // uint8_t, uint64_t
#include <stdio.h>    // printf
#include <sys/stat.h> /* Pour les constantes « mode » */
#include <mqueue.h>
#include <pthread.h>

#include "../../watchdog/watchdog.h"
#include "scanner.h"
#include "../adminScanner/adminScanner.h"
#include "../../utils.h"
#include "../rc522/rfidfct.h"
#include "../../../lib/bcm2835-1.56/bcm2835.h"

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                  TYPEDEF & VARIABLES                                ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

static IdBin idBin;

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                   EXTERN FUNCTIONS                                  ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

extern IdBin Scanner_askIdBin()
{
    while (get_card_info() != TAG_OK)
        usleep(5000);
    idBin.idBin = disp_card_details();
    // p_printf(GREEN, "%s \n", idBin);

    return idBin;
}

extern void Scanner_setIdBin(IdBin idBenne)
{
    idBin = idBenne;
}

extern void Scanner_stop()
{
    if (use_gpio)
        bcm2835_gpio_write(gpio, LOW); // set reset
    bcm2835_spi_end();
    bcm2835_close();
    close_config_file();
}

extern void Scanner_initialise()
{
    if (geteuid() != 0)
    {
        p_printf(RED, "Must be run as root.\n");
        exit(1);
    }
    // catch signals
    set_signals();

    /* read /etc/rc522.conf */
    get_config_file();

    /* set BCM2835 Pins correct */
    HW_init(spi_speed, gpio);
    /* initialise the RC522 */
    InitRc522();
}