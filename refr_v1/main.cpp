/*
 * refr_v1.cpp
 *
 * Created: 04.01.2017 11:39:51
 * Author : Hundd
 */ 


#include "project.h"
#include "ADC.cpp"
#define HIGH_TEMP 25338L
#define LOW_TEMP 28000L
#define VAPORIZE_TEMP 19872L

#define MIN_DELAY_MS 60000				//60000
#define SECONDS_IN_MINUTE 60				//60
#define MIN_COMPRESSOR_WORK_TIME 5		//5
#define COMPRESSOR_HOURS_BEFORE_DEFROST 8//8

void usart_putc(char c)
{
	UDR = c;
	while(!(UCSRA &(1 << UDRE )));
}
void usartPrint(char str[]) {
	int i = 0;
	char c  = str[i++];
	while(c) {
		usart_putc(c);
		c  = str[i++];
	}
}

void usartPrintNumber(long num) {
	int i = 0;
	char c;
	char str[10];
	sprintf(str, "%ld \r\n", num);
	c  = str[i++];
	while(c) {
		usart_putc(c);
		c  = str[i++];
	}
	
}

void usartPrintFloat(float num) {
	int i = 0;
	char c;
	char str[20];
	float integer;
	float mantisa = modff(num,&integer);
	sprintf(str, "%d,%d \r\n", (int)integer,(int)(mantisa*100));
	c  = str[i++];
	while(c) {
		usart_putc(c);
		c  = str[i++];
	}
	
}


//Prototypes
void checkSensors(long*,long*,long*);
void turnOnCompressor();
void turnOffCompressor();
uint8_t cooling ();
int8_t checkCoolingSensor();
void turnOnHeater();
void turnOffHeater();
uint8_t deFroze();




AdcClass adcOb;
int main(void)
{	

	//Температура в холодильной камере
	long coolingTemperature;
	//Температура в морозильной камере
	long freezerTemperature;
	//Температура испарителя
	long vaporizerTemperature;
	sei();
	turnOffCompressor();
	turnOffHeater();
	UCSRB |=1<<TXEN;
	UBRRL=0x33;
    /* Replace with your application code */
	usartPrint((char*)"Starting a program...\r\n");
	
	adcOb.Init();
	adcOb.MesureResistor(PC0);
	//usartPrintNumber(ADC);
	
	
	checkSensors(&coolingTemperature,&freezerTemperature,&vaporizerTemperature);
	usartPrint((char*)"Cooling:   ");
	usartPrintNumber(coolingTemperature);
	usartPrint((char*)"Freezer:   ");
	usartPrintNumber(freezerTemperature);
	usartPrint((char*)"Vaporiser: ");
	usartPrintNumber(vaporizerTemperature);
	
	float* EEP_COMPRESSOR_TIME = (float*)0;
	float totalCompressorTime = 0;

	if(eeprom_read_byte((uint8_t*)EEP_COMPRESSOR_TIME) == 0xFF) {
		eeprom_write_float(EEP_COMPRESSOR_TIME,totalCompressorTime);
	}
	else {
		totalCompressorTime = eeprom_read_float(EEP_COMPRESSOR_TIME);
	}
    while (1) 
    {
		 usartPrintFloat(totalCompressorTime);
		 
		 if (checkCoolingSensor() == -1) {
			totalCompressorTime += cooling()/60.;
			eeprom_write_float(EEP_COMPRESSOR_TIME,totalCompressorTime);
		 }

		 if (totalCompressorTime > COMPRESSOR_HOURS_BEFORE_DEFROST) {
			 deFroze();
			 //Если холодильник разморозился, то счетчик часов компрессора нужно обнулить
			 totalCompressorTime = 0;
			 eeprom_write_float(EEP_COMPRESSOR_TIME,totalCompressorTime);
		 }



		_delay_ms(1000);
    }
}

void checkSensors(long* a, long* b, long* c ) {
	adcOb.MesureResistor(PC0);
	*a = (long)adcOb.Resistor;
	adcOb.MesureResistor(PC1);
	*b = (long)adcOb.Resistor;
	adcOb.MesureResistor(PC2);
	*c = (long)adcOb.Resistor;
}
int8_t checkCoolingSensor(){
	adcOb.MesureResistor(PC0);

	//Нужно выключить компрессор
	if (adcOb.Resistor > LOW_TEMP) {
		return 1;
	}

	//Нужно включить компрессор
	if (adcOb.Resistor < HIGH_TEMP) {
		return -1;
	}
	return 0;
}

int8_t checkVaporizingSensor() {
	adcOb.MesureResistor(PC2);
	if (adcOb.Resistor < VAPORIZE_TEMP) {
			return 1;
	}
	return 0;
}



//Включение и отключение компрессора
uint8_t cooling () {
	uint8_t minutes = 0;
	turnOnCompressor();
	while(checkCoolingSensor() != 1 || minutes < MIN_COMPRESSOR_WORK_TIME) {
		if (minutes < 255) minutes += 1;
		_delay_ms(MIN_DELAY_MS);
		adcOb.MesureResistor(PC0);
	}
	turnOffCompressor();
	return minutes;
}
void turnOnCompressor() {
	DDRD |= 1 << PD2;
	PORTD |= 1 << PD2;
}
void turnOffCompressor() {
	DDRD |= 1 << PD2;
	PORTD &= ~( 1 << PD2 );
}

uint8_t deFroze () {
	unsigned long deFrozeTime = 20;//Minutes
	turnOnHeater();
	while (deFrozeTime) {
		deFrozeTime -=1;
		_delay_ms(SECONDS_IN_MINUTE*1000.);
		if(checkVaporizingSensor()) break;
	}
	turnOffHeater();
	return 0;
}

void turnOnHeater() {
	DDRD |= 1 << PD3;
	PORTD |= 1 << PD3;
}
void turnOffHeater() {
	DDRD |= 1 << PD3;
	PORTD &= ~( 1 << PD3 );
}


