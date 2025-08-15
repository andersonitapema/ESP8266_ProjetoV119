#ifndef SENSOR_H
#define SENSOR_H

#include <DHT.h>

#include "Config.h"
bool errosensor = false;



#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);







// inicialização dos sensores 
void initSensors() {
  dht.begin();  // inicializa o sensor temperatura 
  
}




// sensor de voltagem e corrente
// sensor de voltagem e corrente
void setupINA226() {
  // A função ina226.init() retorna 'true' se a inicialização for bem-sucedida
  // e 'false' se houver falha na comunicação I2C.
  if (ina226.init()) {
    Serial.println("INA226 inicializado com sucesso!");
    ina226.setAverage(AVERAGE_16); 
    ina226.setConversionTime(CONV_TIME_1100);
    ina226.setResistorRange(0.083, 2.0); // choose resistor X mOhm and gain range up to 2 A
    ina226.waitUntilConversionCompleted();
  } else {
    // Se o sensor não for encontrado, esta mensagem será impressa
    // e o código não travará, continuando a execução.
    errosensor = true;
   // logger.log(Logger::ERROR, "Falha ao inicializar INA226. Verifique sua fiação");
    // Você pode adicionar uma variável global aqui para indicar que o sensor
    // não está disponível, se precisar verificar isso em outras partes do código.
  }
}

// --- Leituras ---

float getTemperature() {
  return dht.readTemperature();
}

float getBusVoltage() {
  return ina226.getBusVoltage_V();
}

float getCurrent() {
  return ina226.getCurrent_mA() / 1000.0;  // Retorna a corrente em Ampères
}


float getPower() {
  return ina226.getBusPower() / 1000.0; // Converte miliwatts para watts
}




// --- Status de energia --- atualização dashboard

   String getEnergyStatus() {
  return digitalRead(POWER_PIN) == LOW ? "CONECTADO A REDE ELETRICA" : "SEM REDE ELETRICA";
}



#endif