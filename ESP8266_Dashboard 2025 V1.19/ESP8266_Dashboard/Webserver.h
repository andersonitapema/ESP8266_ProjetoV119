#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESP8266WebServer.h>
#include "MyOTA.h"  //  biblioteca personalizada

#include <ESP8266WebServer.h>
#include "Config.h"
#include "Utilities.h"
#include "MyOTA.h"         // biblioteca personalizada OTA
#include "Websocket.h"
#include "WiFiManage.h"
#include "eeprom.h"        // Salva credenciais do Wi-Fi
#include "bateria.h"
#include "Sensor.h"



// amostrador de bateria exemplo
// Logger logger; // Instância global



// Inicializa o servidor web
ESP8266WebServer server(80);
MyOTA ota(server);

// Variáveis para Wi-Fi, login e restart
String selectedSSID = "";
String enteredPassword = "";

bool isLoginAuthenticated = false;
bool loginFailed = false;
bool shouldRestartESP = false;  // Flag para saber se deve reiniciar
unsigned long restartTimer = 0;  // Timer para reiniciar o ESP


// Função para verificar se o usuário está autenticado
bool isAuthenticated() {
    return isLoginAuthenticated;  // Retorna se o login foi autenticado
}


float ultimaTensao = 0.0;
float ultimaCorrente = 0.0;
float ultimaPotencia = 0.0;

void handleData() {
    String json = "{";
      json += "\"voltage\": " + String(ultimaTensao, 2) + ",";         // Voltagem
      json += "\"amperagem\": " + String(ultimaCorrente, 2) + ",";          // Amperagem
      json += "\"potencia\": " + String(ultimaPotencia, 2) + ",";             // Potência
      json += "\"temperature\": " + String(getTemperature()) + ",";    // Temperatura
      json += "\"energystatus\": \"" + getEnergyStatus() + "\",";      // Status de energia ADICIONADO V1.8
      json += "\"relayState\": \"" + (relayState == HIGH ? String("OFF") : String("ON")) + "\","; // Status do relé ADD V 1.03
      json += "\"relayState2\": \"" + (relayState2 == HIGH ? String("OFF") : String("ON")) + "\","; // Status do relé ADD V 1.04
      json += "\"uptime\": \"" + getFormattedUptime(startTime) + "\","; // Adiciona o uptime
      json += "\"batteryPercentage\":" + String(battery.percentage);     // Adicione a porcentagem da bateria aqui
      json += "}";  // Fecha a chave do JSON
    
    server.send(200, "application/json", json);
}


// Página de Login com erro (se houver)
void handleLoginPage() {
    String loginError = loginFailed ? "<p class='error'>Senha incorreta!</p>" : "";
    String page = R"=====( 
        <!DOCTYPE html>
        <html lang="pt-BR">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Login</title>
            <link rel="stylesheet" href="/style.css">
        </head>
        <body>
            <div class="login-container">
                <div class="login-box">
                    <h2>Login</h2>
                    <form action='/login' method='POST'>
                        <input type='password' name='password' id='password' placeholder='Digite sua senha'>
                        <button type='submit'>Entrar</button>
                    </form>
                    )=====" + loginError + R"=====(
                </div>
            </div>
            <script>
                // Função para alternar a visibilidade da senha
                function togglePasswordVisibility() {
                    var passwordField = document.getElementById('password');
                    if (passwordField.type === 'password') {
                        passwordField.type = 'text';
                    } else {
                        passwordField.type = 'password';
                    }
                }
            </script>
        </body>
        </html>
    )=====";
    server.send(200, "text/html", page);
}

// Função de verificação de login
void handleLogin() {
    loginFailed = false;
    if (server.hasArg("password")) {
        String password = server.arg("password");
        const String correctPassword = "admin"; // Substitua pela sua senha desejada

        String clientIP = server.client().remoteIP().toString(); // Obter IP do cliente

        if (password == correctPassword) {
            isLoginAuthenticated = true;
            // Log de sucesso: Usamos Logger::INFO, que inclui data/hora e é salvo no log.
          // (DESATIVADO)  logger.log(Logger::INFO, "Login bem-sucedido do IP: " + clientIP);
            server.sendHeader("Location", "/dashboard");
            server.send(302, "text/plain", "");
        } else {
            loginFailed = true;
            logger.log(Logger::WARNING, "Tentativa de login falhou do IP: " + clientIP + " (Senha incorreta: " + password + ")");
            server.sendHeader("Location", "/");
            server.send(302, "text/plain", "");
        }
    }
}

// Converte RSSI (em dBm) em uma porcentagem de força de sinal
int rssiToPercentage(int rssi) {
    if (rssi <= -100) {
        return 0;
    } else if (rssi >= -50) {
        return 100;
    } else {
        return 2 * (rssi + 100);  // Mapeia o intervalo de -100 a -50 dBm para 0%-100%
    }
}

// Página do Dashboard com Wi-Fi Status Dinâmico (AP ou Conectado)
void handleDashboard() {
    if (!isAuthenticated()) {
        server.sendHeader("Location", "/");
        server.send(302, "text/plain", "");
        return;
    }

    String wifiStatus = "";
    if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) {
        wifiStatus = "Conectado"; // Quando conectado a um router
    } else {
        wifiStatus = "Access Point"; // Quando em modo AP
    }

    String dashboardPage = R"=====( 
         <!DOCTYPE html>
        <html lang="pt-BR">

        
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css">
            <title>Dashboard</title>
            <link rel="stylesheet" href="/style.css">

            
        
        <script>


       

       function toggleConsole() {
            var consoleContainer = document.getElementById('consoleContainer');
            var button = document.getElementById('minimizeButton');
            
            if (consoleContainer.classList.contains('hidden')) {
                consoleContainer.classList.remove('hidden');
                button.innerText = 'Minimizar Console';
            } else {
                consoleContainer.classList.add('hidden');
                button.innerText = 'Maximizar Console';
            }
        }
        
        // Quando a página carregar, o console já estará minimizado
        window.onload = function() {
            document.getElementById('consoleContainer').classList.add('hidden');
            document.getElementById('minimizeButton').innerText = 'Maximizar Console';
        };


    var socket = new WebSocket('ws://' + window.location.hostname + ':81/');
    socket.onmessage = function(event) {
      document.getElementById("console").value += event.data + "\n";
      document.getElementById("console").scrollTop = document.getElementById("console").scrollHeight; // Rolagem automática
    };
    socket.onopen = function(event) {
      socket.send("getLog#"); // Solicita o histórico de mensagens quando o WebSocket conecta
    };

    function sendCommand() {
      var command = document.getElementById("commandInput").value;
      if (command) {
        socket.send(command);
        document.getElementById("commandInput").value = ''; // Limpa o campo de texto após enviar
      }
    }

    // Função para lidar com a tecla pressionada
    document.addEventListener("DOMContentLoaded", function() {
      document.getElementById("commandInput").addEventListener("keydown", function(event) {
        if (event.key === "Enter") {
          sendCommand(); // Envia o comando quando Enter é pressionado
          event.preventDefault(); // Previne a ação padrão de nova linha
        }
      });
    });

    function fetchData() {
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            if (xhr.readyState == 4 && xhr.status == 200) {
                var data = JSON.parse(xhr.responseText);

                console.log("Dados recebidos:", data);

                // Atualiza os dados de outros elementos
                 document.getElementById('voltage').innerText = data.voltage + ' V';
                  document.getElementById('amperagem').innerText = data.amperagem + ' A';
                  document.getElementById('potencia').innerText = data.potencia + ' W';
                  document.getElementById('temperature').innerText = data.temperature + ' °C';
                  document.getElementById('energystatus').innerText = data.energystatus;
                  document.getElementById('relay1Switch').checked = (data.relayState === "ON");
                  document.getElementById('relay2Switch').checked = (data.relayState2 === "ON");
                  document.getElementById('uptime').innerText = data.uptime;
                  

              

                // Mudar cor com base na tensão
                var voltageElement = document.getElementById('voltage');
                var voltage = parseFloat(data.voltage);
                if (voltage < 11.5) {
                    voltageElement.className = "status-red"; // Vermelho
                } else if (voltage < 12) {
                    voltageElement.className = "status-orange"; // Laranja
                } else if (voltage < 15) {
                    voltageElement.className = "status-green"; // Verde
                } else {
                    voltageElement.className = ""; // Sem classe
                }
            }
        };
        xhr.open('GET', '/data', true);
        xhr.send();
    }


    function toggleRelay(id) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/toggleRelay?id=' + id, true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200) {
            var status = xhr.responseText;
            var isOn = status.includes("ON");
            if (id == 1) {
                document.getElementById('relay1Switch').checked = isOn;
                document.getElementById('relayState').innerText = isOn ? "Desligado" : "Ligado";
            } else if (id == 2) {
                document.getElementById('relay2Switch').checked = isOn;
                document.getElementById('relayState2').innerText = isOn ? "Desligado" : "Ligado";
            }
        }
    };
    xhr.send();
}



   setInterval(fetchData, 1000); // Atualiza a cada 2 segundo
        
        </script>


        </head>
        


        <body>
            <div class="header">
                DASHBOARD 
                <button class="logout-btn small-btn" onclick="window.location.href='/logout';">Sair</button>
            </div>
            

            <div class="grid-container">
                <div class="grid-item" onclick="window.location.href='/wifi';">
                    <h3>Status Wi-Fi</h3>
                    <p>)=====" + wifiStatus + R"=====(</p>
                </div>

            <div class="grid-item">
                <h3><i class="fas fa-stopwatch"></i>
                  Uptime</h3>
                <p id="uptime">0d - 00:00:00</p>
            </div>

               <div class="grid-item">
                <h3><i class="fa fa-plug"></i> Rede Elétrica</h3>
                <p id="energystatus">Desligado</p>
            </div>
              
                  <!-- Caixa de Tensão e Amperagem integrada -->
        <div class="grid-item">
            <div class="voltage-container">
                
                <div class="power-metrics">
                    <div class="metric-row">
                        <div class="metric-label">
                            <i class="fas fa-bolt power-icon"></i>
                            <span>Tensão</span>
                        </div>
                        <div class="metric-value" id="voltage">0 V</div>
                    </div>
                    <div class="metric-row">
                        <div class="metric-label">
                            <i class="fas fa-tachometer-alt power-icon"></i>
                            <span>Corrente</span>
                        </div>
                        <div class="metric-value" id="amperagem">0 A</div>
                    </div>
                    <div class="metric-row">
                        <div class="metric-label">
                            <i class="fas fa-chart-line power-icon"></i>
                            <span>Potência</span>
                        </div>
                        <div class="metric-value" id="potencia">0 W</div>
                    </div>
                </div>
            </div>
        </div>
              
                <div class="grid-item">
                <h3><i class="fa fa-thermometer-half"></i> Temperatura</h3>
                <p id="temperature">0 °C</p>
            </div>


           <!-- Caixa com os dois relés juntos -->
<div class="grid-item">
<h3><i class="fa fa-toggle-on"></i> Controle de Relés</h3>
  <div class="relay-buttons">
    <!-- Relé 1 -->
    <div class="relay-control">
      <strong>Relé:</strong><br>
      <label class="switch">
        <input type="checkbox" id="relay1Switch" onchange="toggleRelay(1)">
        <span class="slider round"></span>
      </label>
    </div>

    <!-- Relé 2 -->
    <div class="relay-control">
      <strong>Relé AUX:</strong><br>
      <label class="switch">
        <input type="checkbox" id="relay2Switch" disabled>  <!-- disabled e sem onchange -->
        <span class="slider round"></span>
      </label>
    </div>
  </div>
</div>

            
              
             
            </div>
          <div class="container">
    <button id="minimizeButton" class="minimize-button" onclick="toggleConsole()">Maximizar Console</button>
    <div id="consoleContainer" class="hidden">
        <h1>Console ESP8266</h1>
        <textarea id="console" readonly></textarea>
        <div class="controls-container">
            <input type="text" id="commandInput" placeholder="Digite um comando" autocapitalize="off" autocomplete="off">
            <button onclick="sendCommand()">Enviar</button>
        </div>
    </div>
</div>
        </body>
        </html>
    )=====";
    server.send(200, "text/html", dashboardPage);
}

// Página de Redes Wi-Fi com RSSI convertido para percentagem e botão 'Mostrar/Ocultar Senha'
void handleScanWifi() {
    if (!isAuthenticated()) {
        server.sendHeader("Location", "/");
        server.send(302, "text/plain", "");
        return;
    }

    String wifiScanPage = R"=====( 
        <!DOCTYPE html>
        <html lang="pt-BR">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Redes Wi-Fi</title>
            <link rel="stylesheet" href="/style.css">
            <style>
                .wifi-container {
                    display: flex;
                    flex-direction: column;
                    align-items: center;
                    justify-content: center;
                    min-height: 100vh;
                    text-align: center;
                }
                .action-buttons {
                    display: flex;
                    gap: 10px;
                    margin-top: 10px;
                }
            </style>
        </head>
        <body>
            <div class="wifi-container">
                <h1>Redes Wi-Fi</h1>
                <form action='/connect-wifi' method='POST'>
    )=====";

    int n = WiFi.scanNetworks();
    if (n == 0) {
        wifiScanPage += "<p>Nenhuma rede detectada!</p>";
    } else {
        wifiScanPage += "<select name='ssid'>";
        for (int i = 0; i < n; ++i) {
            int signalStrength = rssiToPercentage(WiFi.RSSI(i));  // Conversão para porcentagem
            wifiScanPage += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + " (" + String(signalStrength) + "% de sinal)</option>";
        }
        wifiScanPage += "</select><br>";
        wifiScanPage += "<input type='password' id='password' name='password' placeholder='Digite sua senha'><br>";
        wifiScanPage += "<input type='checkbox' onclick='togglePasswordVisibility()'> Mostrar senha<br>";
    }

    wifiScanPage += R"=====( 
        <br>
        <div class="action-buttons">
            <button type='submit'>Conectar</button>
            <button type="button" class="back-btn" onclick="window.location.href='/dashboard';">Voltar</button>
            <button type="button" class="reset-btn" onclick="window.location.href='/reset-wifi';">RESET</button>
        </div>
        </form>
        <script>
            // Função para alternar a visibilidade da senha
            function togglePasswordVisibility() {
                var passwordInput = document.getElementById('password');
                if (passwordInput.type === 'password') {
                    passwordInput.type = 'text';
                } else {
                    passwordInput.type = 'password';
                }
            }
        </script>
        </div>
        </body>
        </html>
    )=====";

    server.send(200, "text/html", wifiScanPage);
}

// Função para conectar ao Wi-Fi e salvar as credenciais
void handleConnectWifi() {
    if (server.hasArg("ssid") && server.hasArg("password")) {
        selectedSSID = server.arg("ssid");
        enteredPassword = server.arg("password");

        WiFi.begin(selectedSSID.c_str(), enteredPassword.c_str());

        String connectResultPage = R"=====( 
            <!DOCTYPE html>
            <html lang="pt-BR">
            <head>
                <meta charset="UTF-8">
                <meta name="viewport" content="width=device-width, initial-scale=1.0">
                <title>Conectando...</title>
                <link rel="stylesheet" href="/style.css">
            </head>
            <body>
                <div class="wifi-container">
                    <h1>Conectando à rede )=====" + selectedSSID + R"=====(</h1>
                    <p>Um momento... Tentando conexão.</p>
        )=====";

        int tryCount = 0;
        while (WiFi.status() != WL_CONNECTED && tryCount < 20) {
            delay(500);
            tryCount++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            // Salvando as Credenciais no SPIFFS 
            // O contador será atualizado dinamicamente de 30 até 0, e então o navegador redirecionará para o endereço IP do ESP8266.
            saveWiFiCredentials(selectedSSID, enteredPassword);
            connectResultPage += "<p>IP: <a href='http://" + WiFi.localIP().toString() + "'>" + WiFi.localIP().toString() + "</a></p>";
            connectResultPage += "<p>Conectado com sucesso! Redirecionando em <span id='counter'>30</span> segundos...</p>";
            connectResultPage += "<script>";
            connectResultPage += "  let count = 30;"; // Início do contador em 30 segundos
            connectResultPage += "  const counter = document.getElementById('counter');";
            connectResultPage += "  const interval = setInterval(() => {";
            connectResultPage += "    count--;"; 
            connectResultPage += "    counter.textContent = count;"; // Atualiza o número na página
            connectResultPage += "    if (count <= 0) {";
            connectResultPage += "      clearInterval(interval);"; // Para o contador
            connectResultPage += "      window.location.href = 'http://" + WiFi.localIP().toString() + "';"; // Redireciona para o IP
            connectResultPage += "    }";
            connectResultPage += "  }, 1000);"; // Intervalo de 1 segundo
            connectResultPage += "</script>";

            
            
            server.send(200, "text/html", connectResultPage);

            // Inicia o temporizador para reiniciar
            shouldRestartESP = true;
            restartTimer = millis();  // Marca o inicio do temporizador
        } else {
            connectResultPage += "<p>Conexão falhou! Tente novamente.</p>";
            connectResultPage += "<br><button class='back-btn small-btn' onclick=\"window.location.href='/dashboard';\">Voltar</button>";
            server.send(200, "text/html", connectResultPage);
        }
    } else {
        server.send(400, F("text/html"), F("<h1>ERRO: SSID ou senha ausentes!</h1>"));
    }
}

// Função para apagar dados Wi-Fi e reiniciar em modo AP
void handleResetWiFi() {
    if (!isAuthenticated()) {
        server.sendHeader("Location", "/");
        server.send(302, "text/plain", "");
        return;
    }
    
    resetWiFiCredentials();
    server.send(200, "text/html", "<h1>Credenciais Wi-Fi apagadas!</h1><br><button onclick=\"window.location.href='/';\">Voltar ao Início</button>");
    ESP.restart();  // Reinicia o ESP depois de limpar as credenciais
}

// Função de logout
void handleLogout() {
    if (isLoginAuthenticated) {
        String clientIP = server.client().remoteIP().toString();
        //logSystemEvent("Logout realizado", "IP: " + clientIP);
    }
    isLoginAuthenticated = false;
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
}

// Função que manipula o CSS no servidor
void handleStyle() {
    String css = R"=====( 
         body {
            font-family: Arial, sans-serif;
            background-color: #1e1e1e;
            color: #ddd;
            margin: 0;
            padding: 0;
        }
        /* Header otimizado */
        .header {
            background-color: #333;
            color: #27ae60;
            padding: 15px;
            text-align: center;
            font-size: 20px;
            position: relative;
          
        }
        /* Botão "Sair" no canto direito, pequeno */
        .logout-btn {
            position: absolute;
            top: 50%;
            right: 10px;
            transform: translateY(-50%);
            padding: 6px 12px;
            background-color: #27ae60;
            border: none;
            color: white;
            cursor: pointer;
            font-size: 14px;
            transition: 0.3s;
        }
        .logout-btn:hover {
            background-color: #219150;
        }
        /* Botões de Ação */
        .back-btn, .reset-btn {
            padding: 8px 12px;
            background-color: #27ae60;
            border: none;
            color: white;
            cursor: pointer;
            transition: 0.3s;
        }
        .back-btn.small-btn {
            font-size: 14px;
            padding: 6px 10px;
        }
        .back-btn:hover, .reset-btn:hover {
            background-color: #219150;
        }
        /* Centralização de login */
        .login-container {
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
        }
        .login-box {
            background-color: #2c2c2c;
            padding: 30px;
            border-radius: 10px;
            text-align: center;
            max-width: 400px;
            width: 100%;
            box-shadow: 0 0 12px rgba(0, 0, 0, 0.6);
        }
        /* Inputs e botões */
        input, button {
            padding: 10px;
            margin: 8px 0;
            width: auto;
            min-width: 120px;
            border-radius: 5px;
            border: none;
            font-size: 16px;
        }
        button {
            background-color: #27ae60;
            color: white;
            cursor: pointer;
            transition: 0.2s;
            margin-left: 20px; /* Adicionei uma margem à esquerda */
        }
        button:hover {
            background-color: #219150;
        }
        
        .relay-buttons {
  display: flex;
  justify-content: center;
  gap: 20px;
  flex-wrap: wrap;
}

.relay-control {
  text-align: center;
}




        /* Mensagem de erro */
        .error {
            color: red;
            margin-top: 10px;
            font-size: 14px;
        }
        /* Centralização de grade no dashboard */
        .grid-container {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
            gap: 20px;
            padding: 20px;
            max-width: 1200px;
            margin: auto;
        }
        .grid-item {
            background-color: #2c2c2c;
            border-radius: 12px;
            padding: 20px;
            text-align: center;
            transition: background-color 0.2s, transform 0.2s;
        }
        .grid-item:hover {
            background-color: #3b3b3b;
        }
        h3 {
            font-size: 18px;
            color: #27ae60;
        }



/* ... (estilos Console ) ... */

.container {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    padding: 20px;
    max-width: 800px;
    margin: 0 auto;
}

#console {
    width: 100%;
    height: 300px;
    border: 1px solid #444;
    border-radius: 8px;
    padding: 15px;
    font-family: monospace;
    background-color: #2c2c2c;
    color: #ddd;
    resize: none;
    margin-bottom: 20px;
    overflow-y: scroll;
    box-sizing: border-box;
}

.controls-container {
    display: flex;
    width: 100%;
    gap: 10px;
    margin-top: 15px;
}

#commandInput {
    flex: 1;
    padding: 12px;
    border: 1px solid #444;
    border-radius: 8px;
    background-color: #2c2c2c;
    color: #ddd;
    font-size: 14px;
}

.minimize-button {
    width: auto;
    min-width: 150px;
    margin-bottom: 20px;
}

button[onclick="sendCommand()"] {
    padding: 12px 24px;
    margin: 0;
    width: auto;
    min-width: 100px;
}

#consoleContainer {
    width: 100%;
    background-color: #1e1e1e;
    border-radius: 12px;
    padding: 20px;
    box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
}

#consoleContainer h1 {
    color: #27ae60;
    text-align: center;
    margin-bottom: 20px;
    font-size: 24px;
}
    
input[type="text"] {
            width: calc(100% - 110px); /* Mantém alinhado com o botão */
    padding: 10px;
    border: 1px solid #ddd;
    border-radius: 5px;
    margin-right: 10px;
    font-size: 16px;
    box-sizing: border-box; /* Garante o mesmo comportamento */
}

.minimize-button {
            background-color: #27ae60;
            color: white;
            padding: 10px 20px;
            font-size: 16px;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            transition: background-color 0.2s;
            margin-bottom: 10px;
        }

        .minimize-button:hover {
            background-color: #219150;
        }

        .hidden {
            display: none;
        }

        /* Switch Toggle estilo */
.switch {
  position: relative;
  display: inline-block;
  width: 50px;
  height: 28px;
}

.switch input {
  opacity: 0;
  width: 0;
  height: 0;
}

.slider {
  position: absolute;
  cursor: pointer;
  top: 0; left: 0; right: 0; bottom: 0;
  background-color: #ccc;
  transition: .4s;
  border-radius: 28px;
}

.slider:before {
  position: absolute;
  content: "";
  height: 22px;
  width: 22px;
  left: 3px;
  bottom: 3px;
  background-color: white;
  transition: .4s;
  border-radius: 50%;
}

input:checked + .slider {
  background-color: #2196F3;
}

input:checked + .slider:before {
  transform: translateX(22px);
}


      
.status-red {
        color: #ff4d4d; /* Um tom de vermelho */
    }

    .status-orange {
        color: #ffa500; /* Um tom de laranja */
    }

    .status-green {
        color: #4CAF50; /* Um tom de verde */
    }



       

        /* Ajustes para telas menores */
        @media (max-width: 600px) {
            .grid-container {
                grid-template-columns: 1fr;
            }
            .login-box {
                max-width: 360px;
            }
            input, button {
                min-width: auto;
            }
        }
    )=====";
    server.send(200, "text/css", css);
}

void handleToggleRelay() {
    if (!server.hasArg("id")) {
        server.send(400, "text/plain", "ID do relé não fornecido");
        return;
    }

    int relayId = server.arg("id").toInt();

    if (relayId == 1) {
        relayState = !relayState;
        digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
     //   Serial.println("Relé 1 alterado para: " + String(relayState ? "ON" : "OFF"));
        server.send(200, "text/plain", "Relé 1 " + String(relayState ? "ON" : "OFF"));
    } else if (relayId == 2) {
        relayState2 = !relayState2;
        digitalWrite(RELAY_PIN2, relayState2 ? HIGH : LOW);
        //Serial.println("Relé 2 alterado para: " + String(relayState2 ? "ON" : "OFF"));
        server.send(200, "text/plain", "Relé 2 " + String(relayState2 ? "ON" : "OFF"));
    } else {
        server.send(400, "text/plain", "ID inválido");
    }
}

 

void setupWebServer() {
    server.on("/", handleLoginPage);
    server.on("/login", handleLogin);
    server.on("/dashboard", handleDashboard);
    server.on("/wifi", handleScanWifi);
    server.on("/connect-wifi", handleConnectWifi);
    server.on("/logout", handleLogout);
    server.on("/reset-wifi", handleResetWiFi);  // Botão para resetar credenciais salvas
    server.on("/style.css", handleStyle);  // Servir o CSS
    server.on("/data", handleData); // Novo endpoint para dados
    server.on("/toggleRelay", handleToggleRelay); // Endpoint para alternar o relé
    
 
 // Inicializa OTA com o servidor web
 ota.begin();  // Inicia a interface OTA personalizada
 // Inicia o servidor
  server.begin();
 // Inicia o WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  
  
}



#endif