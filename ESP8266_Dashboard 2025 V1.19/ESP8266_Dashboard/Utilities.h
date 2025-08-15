#ifndef UTILITIES_H
#define UTILITIES_H

#include <time.h> // Biblioteca para sincronização NTP




unsigned long startTime;   // Variável para rastrear o tempo de início


 


// Configurações do NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -10800; // Ajuste de fuso horário para UTC-3 (Brasília)
const int   daylightOffset_sec = 0; // Horário de verão (3600)1 hora

// Configura a sincronização com o servidor NTP
void horaCerta() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}




// Função para obter a hora e data formatada
String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Falha ao obter o tempo";
  }
  char timeStringBuff[50]; // Buffer de 50 caracteres para armazenar a string formatada
  strftime(timeStringBuff, sizeof(timeStringBuff), "%d-%m-%Y %H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

// Função para obter up time
 String getFormattedUptime(unsigned long startTime) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - startTime;

    unsigned long seconds = (elapsedTime / 1000) % 60;
    unsigned long minutes = (elapsedTime / (1000 * 60)) % 60;
    unsigned long hours = (elapsedTime / (1000 * 60 * 60)) % 24;
    unsigned long days = (elapsedTime / (1000 * 60 * 60 * 24));

    char buffer[20];
    sprintf(buffer, "%ldd - %02ld:%02ld:%02ld", days, hours, minutes, seconds);

    return String(buffer);
}

// verifica sse esta de noite 
bool isNightTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //logger.log(Logger::ERROR, "Falha ao obter hora para verificação de período.");
    return false;  // Evita acionar se a hora estiver indefinida
  }

  int hour = timeinfo.tm_hour;

  // Considera noite entre 18:00 e 06:00
  return (hour >= 18 || hour < 6);
}





#endif