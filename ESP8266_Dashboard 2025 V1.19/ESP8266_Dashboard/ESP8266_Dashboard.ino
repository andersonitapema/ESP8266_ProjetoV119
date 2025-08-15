

#include "Config.h"
#include "eeprom.h"  // Para utilizar EEPROM
#include "Display.h"
#include "Sensor.h"
#include "Utilities.h"
#include "Webserver.h"
#include "Websocket.h"
#include "WiFiManage.h"
#include "bateria.h"

bool autoRelay1Activated = false;
unsigned long energyRestoredTime = 0;
bool waitingToTurnOffRelay = false;




BatteryMonitor battery;  // Cria a instância global







void setup() {

 
  setupParametros();

   
  initSensors(); //inicializando sensor temperatura 

  setupINA226(); // inicializando sensor voltagem e corrente

  carregaEEPROM();

  setupDisplay();

  setupWiFi();

  horaCerta();

  
// Opcional: Configurar o logger para desabilitar ou habilitar serial/websocket
    logger.config(true, true); // Habilita log no serial e websocket
    logger.log(Logger::INFO, "Sistema iniciado.");

 

  
    
  setupWebServer();
  

}


// monitor sensor de rede eletrica 

void checkEnergyStatus() {
    bool currentStatus = digitalRead(POWER_PIN);

    
    // Detecta mudança no estado da energia
    if (currentStatus != lastEnergyStatus) {
        lastEnergyStatus = currentStatus;

        if (currentStatus == HIGH) {  // Falta de energia
            logger.log(Logger::WARNING, "REDE ELETRICA DESCONECTADA");

            if (isNightTime()) {
                logger.log(Logger::INFO, "Período noturno confirmado.");

                if (relayState) {
                    relayState = false;
                    digitalWrite(RELAY_PIN, LOW);
                    autoRelay1Activated = true;
                    logger.log(Logger::DEBUG, "Relé 1 DESLIGADO automaticamente (falta de energia à noite)");
                }
            } else {
                logger.log(Logger::INFO, "Falta de energia fora do horário noturno — relé não foi acionado.");
            }

        } else {  // Energia voltou
            logger.log(Logger::INFO, "REDE ELETRICA RESTAURADA");

            if (autoRelay1Activated) {
                energyRestoredTime = millis();
                waitingToTurnOffRelay = true;
            }
        }
    }

    // Verifica se deve desligar o relé após 10 segundos do retorno da energia
    if (waitingToTurnOffRelay && millis() - energyRestoredTime >= 10000) {
        relayState = true;
        digitalWrite(RELAY_PIN, HIGH);
        autoRelay1Activated = false;
        waitingToTurnOffRelay = false;
        logger.log(Logger::DEBUG, "Relé 1 DESLIGADO automaticamente após retorno da energia (10s)");
    }

    // ➕ NOVA LÓGICA: se a tensão estiver muito baixa, desligue o rele 
    if (ultimaTensao <= 11.1 && !relayState) {
        relayState = true;
        digitalWrite(RELAY_PIN, HIGH);
        logger.log(Logger::INFO, "Tensão baixa, desligando Luz de emergencia.");
    }
}



void atualizaReleAuxiliar() {
    if (ultimaTensao > 13.2) {
        relayState2 = false;
        digitalWrite(RELAY_PIN2, LOW);
    } 
    else if (ultimaTensao < 10.9 && !relayState2) {
        relayState2 = true;
        digitalWrite(RELAY_PIN2, HIGH);
        autoRelay1Activated = false;    // debug -- RESET
        waitingToTurnOffRelay = false;  // debug -- RESET
        logger.log(Logger::INFO, "Tensão muito baixa desligando rele AUX.");

        
    }

}




void sensoresupdate() {
  ultimaTensao = getBusVoltage();
  ultimaCorrente = getCurrent();
  ultimaPotencia = getPower();

}


 

 void loop() {
   static uint8_t flags = 0;  // Usa apenas 1 byte (poderia armazenar 8 flags diferentes)


   // Loop principal do servidor
    server.handleClient();
    webSocket.loop();
    
    
    sensoresupdate(); 
    
    checkEnergyStatus();// verifica status da rede eletrica 

    
    atualizaReleAuxiliar(); // Rotina do rele auxiliar 
   
    updateDisplay(); // atualiza display


    battery.update();   // Atualiza a leitura da bateria

   

   
   
    

    




    
    
    // Executa exatamente quando millis() chegar em 5000
   if (!(flags & 0x01) && (millis() >= 30000)) {  // Verifica o bit 0
    flags |= 0x01;  // Seta o bit 0
    //logger.log(Logger::INFO, "Auto Teste.");

    logger.log(Logger::INFO, "Bateria Lition: " + String(battery.percentage, 2) + "%" " / Bateria Principal: " + String(ultimaTensao, 2) + "V");
    if (errosensor) {
       logger.log(Logger::ERROR, "Falha ao inicializar INA226. Verifique sua fiação");

    }
  }

   

    // Verifica se deve reiniciar o ESP após 10 segundos
    if (shouldRestartESP && (millis() - restartTimer >= 10000)) {
        ESP.restart();
    }

    delay(2);
   


    
}


