#ifndef CONFIG_H
#define CONFIG_H


#include <Wire.h>
#include <INA226_WE.h>  // bliblioteca personalizada 

#define I2C_ADDRESS 0x40



// Definições de pinos


//#define FLASH_BUTTON 0  // GPIO0 é o botão FLASH no ESP8266

#define DHTPIN D4       // Pino onde o DHT11 está conectado
#define POWER_PIN D7  // Pino para monitorar a presença energia 
#define RELAY_PIN D1  // Led Azul pcb   PCB nomeclarura D4
#define RELAY_PIN2 D2  // Led Azul pcb   PCB nomeclarura D4


INA226_WE ina226 = INA226_WE(I2C_ADDRESS);


bool relayState = HIGH;  // Estado inicial do relé
bool relayState2 = HIGH;  // Estado inicial do relé
bool lastEnergyStatus = LOW; // Estado inicial status de energia 







void setupParametros() {
  
  Wire.begin(14, 12);  // SDA no pino D5 (GPIO14), SCL no pino D6 (GPIO12)
  
  Serial.begin(115200);

  pinMode(POWER_PIN, INPUT_PULLUP);  // Usar pull-up interno para detectar falhas de energia

  
  

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RELAY_PIN2, OUTPUT);
  digitalWrite(RELAY_PIN, relayState);  // confuguraçao inicial do status do rele 
  digitalWrite(RELAY_PIN2, relayState2);  // confuguraçao inicial do status do rele 2 
  
}


  



  




#endif