/*
  Projeto: ESP8266 - Login com senha e token de sessão
  Autor: Anderson Coelho
  Data: 14/08/2025
  Versão: 1.0

  Descrição:
  Este sketch implementa um servidor web no ESP8266 com:
    - Tela de login apenas com senha
    - Token de sessão gerado aleatoriamente
    - Página de boas-vindas após login
    - Dashboard protegido por autenticação

  Bibliotecas utilizadas:
    - ESP8266WiFi.h
    - ESP8266WebServer.h

  Configurações importantes:
    - SSID e senha da rede Wi-Fi no setup()
    - Senha de login configurada na variável 'adminPass'

    
*/


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* adminPass = "1234";
String sessionToken = "";

ESP8266WebServer server(80);

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
  String html = "<html><body><h2>Login</h2>"
                "<form action='/login' method='POST'>"
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
      Serial.println("[INFO] Token gerado: " + sessionToken);

      // Monta página de boas-vindas com cookie
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
