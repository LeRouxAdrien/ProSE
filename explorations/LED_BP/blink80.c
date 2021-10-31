/* -----------------------------------------------------------------------------
Programme classique LED clignotante
Led rouge sur GPIO23 (4) via une résistance de 330 ohms
os                  : RPi Linux 4.4.13+
logiciel            : gcc (Raspbian 4.9.2-10) 4.9.2
cible               : raspberry Pi
date de création    : 21/06/2016
date de mise à jour : 22/06/2016
version             : 1.0
auteur              : icarePetibles
référence           : www.wiringpi.com
Remarques           :
----------------------------------------------------------------------------- */
/* -----------------------------------------------------------------------------
Bibliothèques
----------------------------------------------------------------------------- */
#include <stdio.h>                     //utilisé pour printf()
#include <wiringPi.h>                  //bibliothèque wiringPi
//------------------------------------------------------------------------------
#define LED 4                          //numéro led = GPIO23
int main(void){                        //programme principale
    printf("Led clignotante\n");       //IHM
    wiringPiSetup();                   //numérotation wiringPi ou "pseudo Arduino"
    pinMode(LED, OUTPUT);              //pin en sortie
    for(;;){                           //boucle infinie
        digitalWrite(LED, HIGH);       //allume led
        delay (500);                   //attente 0.5 sec
        digitalWrite(LED, LOW);        //éteint led
        delay (500);                   //attente 0.5 sec
    }
    return(0);                         //code sortie
}
//------------------------------------------------------------------------------