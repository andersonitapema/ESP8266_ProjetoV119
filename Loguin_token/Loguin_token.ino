/*
  Projeto: ESP8266 - Login com senha e token de sessão
  Autor: Anderson Coelho (ChatGBT)
  Data: 14/08/2025
  Versão: 2.0

  Descrição:
  Este sketch implementa um servidor web no ESP8266 com:
    - Tela de login apenas com senha
    - Token de sessão gerado aleatoriamente
    - Expiração automática do token (5 minutos)
    - Página de boas-vindas após login
    - Logout funcional com invalidação do token
    - Dashboard protegido por autenticação

  Bibliotecas utilizadas:
    - ESP8266WiFi.h
    - ESP8266WebServer.h

  Configurações importantes:
    - SSID e senha da rede Wi-Fi no setup()
    - Senha de login configurada na variável 'adminPass'
    - Tempo de expiração do token definido em 'TOKEN_EXPIRATION'

 
*/


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* adminPass = "1234";
String sessionToken = "";
unsigned long tokenTime = 0;           
const unsigned long TOKEN_EXPIRATION = 5 * 60 * 1000; // 5 minutos em milissegundos

ESP8266WebServer server(80);

// Variável para mostrar mensagem de sessão expirada
bool sessionExpired = false;

// ===== Gera token aleatório =====
String generateSessionToken() {
  String token = "";
  for (int i = 0; i < 16; i++) {
    token += char(random(65, 91)); // Letras A-Z
  }
  return token;
}

// ===== Verifica autenticação =====
bool isAuthenticated() {
  if (sessionToken == "") return false;

  // Verifica expiração
  if (millis() - tokenTime > TOKEN_EXPIRATION) {
    Serial.println("[INFO] Token expirou");
    sessionToken = "";
    sessionExpired = true;
    return false;
  }

  if (server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
    if (cookie.indexOf("session=" + sessionToken) != -1) {
      return true;
    }
  }
  return false;
}

// ===== Página de login =====
void handleLoginPage() {
  String html = "<html><body><h2>Login</h2>";
  
  if (sessionExpired) {
    html += "<p style='color:red;'>Sessão expirou! Por favor, faça login novamente.</p>";
    sessionExpired = false; // Reseta a flag
  }

  html += "<form action='/login' method='POST'>"
          "Senha: <input type='password' name='password'><br>"
          "<input type='submit' value='Entrar'>"
          "</form></body></html>";
  server.send(200, "text/html", html);
}

// ===== Processa login =====
void handleLogin() {
  if (server.method() == HTTP_POST) {
    String password = server.arg("password");

    if (password == adminPass) {
      sessionToken = generateSessionToken();
      tokenTime = millis();  // marca o horário do login
      Serial.println("[INFO] Token gerado: " + sessionToken);

      // Página de boas-vindas com cookie
      String html = "<html><body>"
                    "<h1>🎉 Bem-vindo!</h1>"
                    "<p>Senha aceita com sucesso.</p>"
                    "<p><a href='/'>Ir para o Dashboard</a></p>"
                    "<p><a href='/logout'>Sair</a></p>"
                    "<script>"
                    "document.cookie='session=" + sessionToken + "; path=/;';"
                    "</script>"
                    "</body></html>";
      server.send(200, "text/html", html);
      return;
    }
  }
  server.send(401, "text/plain", "Senha incorreta!");
}

// ===== Página principal =====
void handleRoot() {
  if (!isAuthenticated()) {
    handleLoginPage();
    return;
  }
  server.send(200, "text/html", "<h1>Dashboard</h1><a href='/logout'>Sair</a>");
}

// ===== Logout =====
void handleLogout() {
  sessionToken = "";
  server.sendHeader("Set-Cookie", "session=; Max-Age=0; Path=/");
  handleLoginPage();
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(A0));

  WiFi.begin("C3T", "0102030405");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/login", HTTP_GET, handleLoginPage);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/logout", handleLogout);

  server.begin();
}

void loop() {
  server.handleClient();
}
