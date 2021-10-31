#include <stdio.h>
#include <wiringPi.h>
#include <time.h>
#define LED 27
#define pinBtn 8

int main(void){
	
	wiringPiSetup();
	pinMode(LED, OUTPUT);
	pinMode(pinBtn, INPUT);
	
	while (1){
		int etat = digitalRead(pinBtn);
		if(etat == 0){
			printf("appuie détecter");
			digitalWrite(LED, HIGH);
		
		}
		else{
			digitalWrite(LED,LOW);
		}
	sleep(0.3);
	
	}
	return 0;
	
}

