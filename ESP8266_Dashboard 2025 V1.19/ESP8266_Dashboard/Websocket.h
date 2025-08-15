#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <WebSocketsServer.h>
#include <IPAddress.h>
#include <ESP8266Ping.h>
#include "eeprom.h"
#include "Utilities.h"
#include "Webserver.h"
#include "WiFiManage.h"
#include "bateria.h"

WebSocketsServer webSocket = WebSocketsServer(81); // Porta do WebSocket

// =============================================
// SEÇÃO DO LOGGER (integrada diretamente)
// =============================================
class Logger {
private:
    bool _serialEnabled = true;
    bool _websocketEnabled = true;
    static const int LOG_CAPACITY = 20;

    // Buffer de mensagens (substitui o messageLog antigo)
    String _messageLog[LOG_CAPACITY];
    int _logIndex = 0;

public:
    enum Level { DEBUG, INFO, WARNING, ERROR };

    // Função para adicionar uma mensagem ao log (interno à classe Logger)
    void addLogMessage(const String& message) {
        _messageLog[_logIndex] = message;
        _logIndex = (_logIndex + 1) % LOG_CAPACITY; // Roda o índice para armazenar no máximo 20 mensagens
    }

    void log(Level level, const String& message) {
        String logEntry;
        if (level == DEBUG || level == ERROR) {
            // DEBUG e ERROR não mostram data/hora e não salvam no log
            logEntry = String("[") + getLevelString(level) + "] " + message;
        } else {
            // INFO e WARNING mostram data/hora e salvam no log
            logEntry = String("[") + getLevelString(level) + "] " + getFormattedTime() + " - " + message;
            addLogMessage(logEntry); // Adiciona ao log apenas INFO e WARNING
        }

        // 1. Saída para Serial
        if (_serialEnabled) Serial.println(logEntry);

        // 2. Envia via WebSocket para todos os clientes
        if (_websocketEnabled) webSocket.broadcastTXT(logEntry);
    }

    // Método para acesso ao histórico
    String* getMessageLog() { return _messageLog; }
    int getLogIndex() { return _logIndex; }

    // Configuração de onde o log será exibido
    void config(bool serial, bool websocket) {
        _serialEnabled = serial;
        _websocketEnabled = websocket;
    }

    // Método para enviar o histórico de log para um cliente específico
    void sendLogToClient(uint8_t num) {
    int index = _logIndex; // Começa do log mais antigo
    for (int i = 0; i < LOG_CAPACITY; i++) {
        int currentIndex = (index + i) % LOG_CAPACITY;
        if (_messageLog[currentIndex] != "") {
            webSocket.sendTXT(num, _messageLog[currentIndex]);
        }
    }
}


private:
    String getLevelString(Level level) {
        switch(level) {
            case DEBUG: return "DEBUG";
            case INFO: return "INFO";
            case WARNING: return "WARNING";
            case ERROR: return "ERROR";
            default: return "";
        }
    }
};

extern Logger logger; // Declara uma instância global do Logger para ser usada em outros arquivos

// =============================================
// SEÇÃO PRINCIPAL DO WEBSOCKET
// =============================================


// Função para interpretar comandos recebidos pelo WebSocket
void handleCommand(String command) {
    // Comando para solicitar o histórico de mensagens
    if (command == "getLog#") {
        logger.sendLogToClient(0); // Envia para o cliente 0 (assumindo que é o único)
        return;
    }

    if (command.startsWith("ip ")) {
        String newIpString = command.substring(3);
        IPAddress newIp;

        if (newIp.fromString(newIpString)) {
            if (WiFi.config(newIp, WiFi.gatewayIP(), WiFi.subnetMask())) {
                logger.log(Logger::INFO, "Novo IP configurado: " + newIpString);
            } else {
                logger.log(Logger::ERROR, "Falha ao configurar o novo IP.");
            }
        } else {
            logger.log(Logger::ERROR, "Formato de IP inválido.");
        }
        return;
    }

    if (command == F("ajuda")) {
        String message;
        message += F("--- Comandos Disponíveis ---\n");
        message += F("1. ajuda: Mostra esta lista de comandos.\n");
        message += F("2. busca wifi: Lista redes Wi-Fi disponíveis.\n");
        message += F("3. wifi: Exibe informações da rede Wi-Fi atual (SSID, IP, Sinal).\n");
        message += F("4. reinicia: Reinicia o ESP8266.\n");
        message += F("5. limpar: Limpa o histórico de logs no console.\n");
        message += F("6. infobateria: Exibe a tensão e porcentagem da bateria.\n");
        message += F("7. ping <IP>: Testa a conexão com um IP (ex: ping 8.8.8.8).\n");
        message += F("8. getLog: Exibe o histórico de logs (apenas INFO e WARNING).\n");
        message += F("9. ip <novo_ip>: Configura um IP estático (ex: ip 192.168.1.100).\n");
        webSocket.broadcastTXT(message);
    }


else if (command == "listar arquivos") {
    String files = listTxtFiles();
    String output = files; // Cria variável explícita
    webSocket.broadcastTXT(output);
}

else if (command.startsWith("ler arquivo ")) {
    String filename = command.substring(12);
    String content = readFileContent(filename);
    String output = "Conteúdo de " + filename + ":\n" + content;
    webSocket.broadcastTXT(output);
}

    else if (command == "busca wifi") {
        int numNetworks = WiFi.scanNetworks();
        String message = "Redes encontradas:\n";

        for (int i = 0; i < numNetworks; i++) {
            String ssid = WiFi.SSID(i);
            int rssi = WiFi.RSSI(i);
            int percentage = map(rssi, -100, -30, 0, 100);
            percentage = constrain(percentage, 0, 100);

            message += String(i + 1) + ". " + ssid + " (Sinal: " + String(percentage) + "%)\n";
           //logger.log(Logger::INFO, ssid + " (Sinal: " + String(percentage) + "%)");
        }
        webSocket.broadcastTXT(message);
        return;
    }

    else if (command == "wifi") {
        String ssid = WiFi.SSID();
        String ip = WiFi.localIP().toString();
        int rssi = WiFi.RSSI();
        int signalPercentage = map(rssi, -100, -50, 0, 100);
        signalPercentage = constrain(signalPercentage, 0, 100);
        String message = "Rede: " + ssid + ", IP: " + ip + ", Potência: " + String(signalPercentage) + "%";
        logger.log(Logger::INFO, message); // Loga como INFO
        webSocket.broadcastTXT(message);
    }

    else if (command == "reinicia") {
        logger.log(Logger::WARNING, "Reiniciando o ESP8266...");
        delay(1000);
        ESP.restart();
    }

    else if (command == "limpar") {
        // Para limpar o log, basta re-inicializar a instância do Logger ou chamar um método de reset
        for (int i = 0; i < 20; i++) {
            logger.getMessageLog()[i] = ""; // Limpa cada posição do log
        }
        logger.log(Logger::INFO, "Logs limpos com sucesso.");
    }

    else if (command == "infobateria") {
        String message = "Bateria: " + String(battery.voltage, 2) + " V (" + String(battery.percentage, 0) + "%)";
        logger.log(Logger::DEBUG, message); // Opcional: logar DEBUG da bateria
    }

    else if (command.startsWith("ping ")) {
        String ipString = command.substring(5);
        IPAddress ipToPing;

        if (ipToPing.fromString(ipString)) {
            bool success = Ping.ping(ipToPing, 3);
            String resultMessage;
            if (success) {
                resultMessage = "Ping em " + ipString + " bem-sucedido!";
                logger.log(Logger::INFO, resultMessage);
            } else {
                resultMessage = "Falha no ping para " + ipString;
                logger.log(Logger::ERROR, resultMessage);
            }
            webSocket.broadcastTXT(resultMessage);
        } else {
            logger.log(Logger::ERROR, "Formato de IP inválido.");
            webSocket.broadcastTXT("Formato de IP inválido");
        }
    } else {
        logger.log(Logger::ERROR, "Comando inválido: " + command); // Loga comandos inválidos como ERROR
        webSocket.broadcastTXT("Comando inválido");
    }
}

// Callback de WebSocket para lidar com mensagens recebidas
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
        String command = String((char *)payload);
        handleCommand(command);
    }
}




// Instância global do logger
Logger logger;

#endif