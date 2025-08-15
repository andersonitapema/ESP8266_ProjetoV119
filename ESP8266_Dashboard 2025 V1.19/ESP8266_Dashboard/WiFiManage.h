#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H


#include <ESP8266WiFi.h>
#include <FS.h>  // Para utilizar o sistema de arquivos SPIFFS

#include "eeprom.h"






// Função para conectar à rede Wi-Fi salva ou iniciar no modo AP
void setupWiFi() {
  
     String loadedSSID, loadedPassword;
    if (loadWiFiCredentials(loadedSSID, loadedPassword)) {
        // Tenta conectar à rede salva
        WiFi.begin(loadedSSID.c_str(), loadedPassword.c_str());

        Serial.println("Tentando conectar à última rede Wi-Fi conhecida...");

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConectado com sucesso: " + WiFi.SSID());
          //  Serial.print("Endereço IP: "); Ja existe essa linha despois de "Servidor iniciado"
            Serial.println(WiFi.localIP());
            return;
        }

        Serial.println("\nConexão falhou. Iniciando modo AP.");
    } else {
        Serial.println("Nenhuma credencial Wi-Fi encontrada.");
    }

    // MODO AP (com DHCP automático)
    WiFi.mode(WIFI_AP);
    
    IPAddress local_IP(192, 168, 4, 1);       // IP do AP
    IPAddress gateway(192, 168, 4, 1);        // Gateway do AP
    IPAddress subnet(255, 255, 255, 0);       // Máscara de rede

    WiFi.softAPConfig(local_IP, gateway, subnet);  // Configura IP do AP
    WiFi.softAP("ESP8266-Dashboard");

    Serial.println("Modo AP iniciado. SSID: ESP8266-Dashboard");
    Serial.print("IP do Ponto de Acesso: ");
    Serial.println(WiFi.softAPIP());

}







#endif