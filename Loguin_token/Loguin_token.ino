/*
  Projeto: ESP8266 - Login Moderno com Token Seguro
  Versão: 3.4 (Funcional com Token via URL)
  Autor: Anderson Oxsystem
  
  Descrição geral:
  - Login apenas com senha, interface moderna com Tailwind CSS.
  - Token de sessão gerado no ESP8266.
  - Token expira automaticamente após 5 minutos.
  - Dashboard protegido, logout funcional.
  - Token passado via URL para evitar problemas com cookies + fetch().
*/

#include <ESP8266WiFi.h>         // Biblioteca para conexão WiFi
#include <ESP8266WebServer.h>    // Biblioteca para criar servidor web

// ---------- CONFIGURAÇÕES DE LOGIN ----------
const char* adminPass = "1234";   // Senha de administrador
String sessionToken = "";          // Token de sessão atual
unsigned long tokenTime = 0;       // Momento do último login
const unsigned long TOKEN_EXPIRATION = 5*60*1000; // 5 minutos em ms

// ---------- INICIALIZAÇÃO DO SERVIDOR ----------
ESP8266WebServer server(80);

// ---------- FUNÇÃO: GERA UM TOKEN ALEATÓRIO ----------
String generateToken(){
  String token = "";
  for(int i = 0; i < 16; i++){
    token += char(random(65, 91)); // Letras A-Z
  }
  return token;
}

// ---------- FUNÇÃO: VERIFICA SE O TOKEN É VÁLIDO ----------
bool isAuthenticated(String token){
  // Verifica se o token corresponde ao gerado e se não expirou
  if(token == sessionToken && (millis() - tokenTime) <= TOKEN_EXPIRATION) 
    return true;
  return false;
}

// ---------- FUNÇÃO: PÁGINA DE LOGIN ----------
void handleLoginPage(){
  // HTML da página de login
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Login</title>
<script src="https://cdn.tailwindcss.com"></script>
</head>
<body class="bg-gray-900 text-white flex items-center justify-center min-h-screen">
<div class="w-full max-w-sm bg-gray-800 p-8 rounded-xl">
<h1 class="text-2xl font-bold mb-4">Acesso Restrito</h1>
<form id="loginForm">
<label>Senha</label>
<input type="password" id="password" required class="w-full mb-4 p-2 rounded bg-gray-700"><br>
<button type="submit" class="w-full bg-indigo-600 p-2 rounded">Entrar</button>
</form>
<div id="msg" class="mt-2"></div>
<script>
document.getElementById('loginForm').addEventListener('submit', async function(e){
  e.preventDefault(); // Evita que a página recarregue
  let pwd = document.getElementById('password').value;
  try{
    const r = await fetch('/login', {
      method: 'POST',
      headers: {'Content-Type':'application/x-www-form-urlencoded'},
      body: 'password=' + encodeURIComponent(pwd)
    });
    if(r.ok){
      const token = await r.text();
      window.location.href='/dashboard?token=' + token; // Redireciona com token
    } else {
      const t = await r.text();
      document.getElementById('msg').innerHTML = '<span class="text-red-400">'+t+'</span>';
    }
  } catch(e){
    document.getElementById('msg').innerHTML = '<span class="text-red-400">Erro de conexão</span>';
  }
});
</script>
</div>
</body>
</html>
)rawliteral";

  server.send(200,"text/html",page); // Envia página para o cliente
}

// ---------- FUNÇÃO: PROCESSA LOGIN POST ----------
void handleLogin(){
  if(server.method() == HTTP_POST){
    String pwd = server.arg("password"); // Captura senha enviada
    if(pwd == adminPass){
      sessionToken = generateToken();    // Gera token novo
      tokenTime = millis();              // Armazena hora do login
      Serial.println("[INFO] Token gerado: "+sessionToken);
      server.send(200,"text/plain",sessionToken); // Retorna token para JS
      return;
    }
  }
  // Senha incorreta ou método incorreto
  server.send(401,"text/plain","Senha incorreta!");
}

// ---------- FUNÇÃO: DASHBOARD ----------
void handleDashboard(){
  String token = server.arg("token");    // Pega token da URL
  if(!isAuthenticated(token)){
    handleLoginPage();                   // Se inválido, retorna para login
    return;
  }

  // HTML do dashboard
  String dash = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Dashboard</title>
<script src="https://cdn.tailwindcss.com"></script>
</head>
<body class="bg-gray-900 text-white p-4">
<h1 class="text-3xl text-indigo-400 mb-4">Dashboard</h1>
<p>Bem-vindo! A sessão expira em 5 minutos.</p>
<a href="/logout" class="bg-red-600 p-2 rounded mt-4 inline-block">Sair</a>
</body>
</html>
)rawliteral";

  server.send(200,"text/html",dash);
}

// ---------- FUNÇÃO: LOGOUT ----------
void handleLogout(){
  sessionToken = "";         // Invalida token
  handleLoginPage();         // Redireciona para login
}

// ---------- FUNÇÃO: SETUP ----------
void setup(){
  Serial.begin(115200);
  randomSeed(analogRead(A0));   // Inicializa gerador aleatório

  // Conexão WiFi
  WiFi.begin("C3T","0102030405");
  Serial.print("Conectando");
  while(WiFi.status() != WL_CONNECTED){ delay(500); Serial.print("."); }
  Serial.println("\nConectado: "+WiFi.localIP().toString());

  // Rotas do servidor
  server.on("/", handleLoginPage);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/dashboard", handleDashboard);
  server.on("/logout", handleLogout);

  server.begin();
  Serial.println("Servidor iniciado");
}

// ---------- LOOP PRINCIPAL ----------
void loop(){
  server.handleClient();   // Mantém o servidor escutando requisições
}
