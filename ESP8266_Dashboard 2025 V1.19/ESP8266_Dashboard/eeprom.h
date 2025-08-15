#ifndef EEPROM_H
#define EEPROM_H

#include <FS.h>  // Para utilizar o sistema de arquivos SPIFFS



// Função para listar arquivos TXT (adicione esta função)
String listTxtFiles() {
    String fileList = "Arquivos TXT encontrados:\n";
    Dir dir = SPIFFS.openDir("/");
    
    while (dir.next()) {
        if (dir.isFile() && dir.fileName().endsWith(".txt")) {
            fileList += "- " + dir.fileName() + " (" + String(dir.fileSize()) + " bytes)\n";
        }
    }
    
    if (fileList == "Arquivos TXT encontrados:\n") {
        fileList = "Nenhum arquivo TXT encontrado na memória.\n";
    }
    
    return fileList;
}

// Função para ler conteúdo de arquivo (adicione esta função)
String readFileContent(const String &filename) {
    if (!SPIFFS.exists(filename)) {
        return "Arquivo não encontrado: " + filename;
    }

    File file = SPIFFS.open(filename, "r");
    if (!file) {
        return "Erro ao abrir o arquivo: " + filename;
    }

    String content = file.readString();
    file.close();
    return content;
}



// Função para carregar as credenciais Wi-Fi
bool loadWiFiCredentials(String &ssid, String &password) {
    File file = SPIFFS.open("/wifi.txt", "r");
    if (!file) {
        Serial.println("Falha ao abrir arquivo Wi-Fi");
        return false;
    }
    ssid = file.readStringUntil('\n');
    ssid.trim();
    password = file.readStringUntil('\n');
    password.trim();
    file.close();
    return true;
}

// Função para salvar as credenciais Wi-Fi
bool saveWiFiCredentials(const String &ssid, const String &password) {
    File file = SPIFFS.open("/wifi.txt", "w");
    if (!file) {
        Serial.println("Falha ao abrir arquivo Wi-Fi para escrita");
        return false;
    }
    file.println(ssid);
    file.println(password);
    file.close();
    return true;
}

// Inicializa o SPIFFS
void carregaEEPROM(){
    if (!SPIFFS.begin()) {
        Serial.println("Falha ao montar o sistema de arquivos; reiniciando...");
        ESP.restart();
    }
}


// Função para apagar credenciais Wi-Fi do SPIFFS (Reset)
void resetWiFiCredentials() {
    if (SPIFFS.exists("/wifi.txt")) {
        SPIFFS.remove("/wifi.txt");
        Serial.println("Credenciais Wi-Fi apagadas");
    } else {
        Serial.println("Nenhuma credencial salva foi encontrada.");
    }
}




#endif