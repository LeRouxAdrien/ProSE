#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

int PinCLK = 3;
int PinDT = 5;
int PinSW = 7;
static long encoderPos = 0;
int PinSWLast = 1, PinSWNew = 1; // 0 = bouton appuyé, 1 = bouton non appyé
int PinCLKLast = 1, PinCLKNew = 1; // 0 = entre 2 crans, 1 = sur un cran
int nbPas = 20;                 // Résolution de l'encodeur

int main(void)
{
	if(wiringPiSetupPhys()==-1)
	{
		fprintf(stderr, "Erreur lors de l'initialisation de la bibliothèque wiringPi\n");
		return EXIT_FAILURE;
	}
	pinMode (PinCLK,INPUT);
	pinMode (PinDT,INPUT);
	pinMode (PinSW,INPUT);
    printf("Lancement du scan\n");

	while(1) {
		PinSWNew = digitalRead(PinSW);

		if (PinSWLast && !PinSWNew) {      // Reset la position si on appui sur le potentiomètre
			encoderPos = 0;
 			printf("Reset position\n");
 		}
   		
		PinCLKNew = digitalRead(PinCLK);
		
 		if (!PinCLKLast && PinCLKNew) {
 			if (digitalRead(PinDT) == 1) {
 				printf("Sens horaire, position ");
				encoderPos++;
				if ( encoderPos > ( nbPas - 1 ) ) {
					encoderPos = 0;
				}
                printf("%ld", encoderPos);
                printf(", angle ");
                printf("%d\n",  abs ( encoderPos * ( 360 / nbPas ) ) );
 			}
		}
		PinSWLast = PinSWNew;
		PinCLKLast = PinCLKNew;
	}

	return EXIT_SUCCESS;
}
