/*
  Projeto: ESP8266 - Login Moderno com Token Seguro
  Versão: 3.4 (Funcional com Token via URL)

  Descrição:
  - Tela de login moderna usando Tailwind CSS.
  - Login apenas com senha (não exibe usuário na interface).
  - Token de sessão gerado aleatoriamente pelo ESP8266.
  - Expiração automática do token em 5 minutos.
  - Dashboard protegido que só permite acesso se o token for válido.
  - Logout funcional que invalida o token.
  - JavaScript ajustado para:
      * Receber o token do servidor após login.
      * Redirecionar para o Dashboard usando token via URL.
      * Exibir mensagens dinâmicas de sucesso ou erro.
  - Problema de cookie/fetch resolvido usando token via URL.
  - Código organizado para boas práticas:
      * Separação clara entre páginas HTML e lógica do servidor.
      * Funções pequenas e específicas: login, dashboard, logout.
      * Uso de constantes para configuração de senha e tempo de expiração.
      * Serial monitor para debug do token gerado.

  Observações:
  - Funciona com ESP8266WebServer.
  - Segurança básica adequada para uso local em rede privada.
  - Pode ser estendido para cookies ou autenticação mais avançada.
*/


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* adminPass = "1234";
String sessionToken = "";
unsigned long tokenTime = 0;
const unsigned long TOKEN_EXPIRATION = 5*60*1000; // 5 minutos

ESP8266WebServer server(80);

String generateToken(){
  String token="";
  for(int i=0;i<16;i++) token+=char(random(65,91));
  return token;
}

bool isAuthenticated(String token){
  if(token==sessionToken && (millis()-tokenTime)<=TOKEN_EXPIRATION) return true;
  return false;
}

// Página de login
void handleLoginPage(){
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Login</title><script src="https://cdn.tailwindcss.com"></script>
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
document.getElementById('loginForm').addEventListener('submit',async function(e){
e.preventDefault();
let pwd=document.getElementById('password').value;
try{
let r=await fetch('/login',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'password='+encodeURIComponent(pwd)});
if(r.ok){
let token=await r.text();
window.location.href='/dashboard?token='+token;
}else{
let t=await r.text();
document.getElementById('msg').innerHTML='<span class="text-red-400">'+t+'</span>';
}
}catch(e){
document.getElementById('msg').innerHTML='<span class="text-red-400">Erro de conexão</span>';
}
});
</script>
</div>
</body>
</html>
)rawliteral";
  server.send(200,"text/html",page);
}

// Login POST
void handleLogin(){
  if(server.method()==HTTP_POST){
    String pwd=server.arg("password");
    if(pwd==adminPass){
      sessionToken=generateToken();
      tokenTime=millis();
      Serial.println("[INFO] Token gerado: "+sessionToken);
      server.send(200,"text/plain",sessionToken);
      return;
    }
  }
  server.send(401,"text/plain","Senha incorreta!");
}

// Dashboard
void handleDashboard(){
  String token=server.arg("token");
  if(!isAuthenticated(token)){
    handleLoginPage();
    return;
  }
  String dash = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Dashboard</title><script src="https://cdn.tailwindcss.com"></script></head>
<body class="bg-gray-900 text-white p-4">
<h1 class="text-3xl text-indigo-400 mb-4">Dashboard</h1>
<p>Bem-vindo! A sessão expira em 5 minutos.</p>
<a href="/logout" class="bg-red-600 p-2 rounded mt-4 inline-block">Sair</a>
</body></html>
)rawliteral";
  server.send(200,"text/html",dash);
}

// Logout
void handleLogout(){
  sessionToken="";
  handleLoginPage();
}

void setup(){
  Serial.begin(115200);
  randomSeed(analogRead(A0));
  WiFi.begin("C3T","0102030405");
  Serial.print("Conectando");
  while(WiFi.status()!=WL_CONNECTED){delay(500); Serial.print(".");}
  Serial.println("\nConectado: "+WiFi.localIP().toString());

  server.on("/",handleLoginPage);
  server.on("/login",HTTP_POST,handleLogin);
  server.on("/dashboard",handleDashboard);
  server.on("/logout",handleLogout);

  server.begin();
  Serial.println("Servidor iniciado");
}

void loop(){server.handleClient();}
