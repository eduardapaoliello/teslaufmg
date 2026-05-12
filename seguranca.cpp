#include <math.h>
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// =============================================
// CONFIGURAÇÕES DE REDE
// =============================================
const char* ssid      = "SUA_REDE_WIFI";
const char* password  = "SUA_SENHA_WIFI";
const char* serverURL = "http://192.168.68.105:5000/dados"; // IP do seu PC

// =============================================
// MAPEAMENTO DE PINOS
// =============================================
const int pinoVBat     = 34;   // Divisor de tensão → ADC
const int pinoCorrente = 35;   // Shunt + op-amp    → ADC
const int pinoTemp     = 32;   // NTC temperatura   → ADC

const int pinoMotorA1  = 12;
const int pinoMotorA2  = 13;
const int pinoMotorB1  = 14;
const int pinoMotorB2  = 27;
const int pinoEna      = 33;
const int pinoEnb      = 26;

const int pinoLedR     = 25;
const int pinoLedG     = 19;
const int pinoBuzzer   = 18;

// =============================================
// CONSTANTES DO DIVISOR DE TENSÃO
// R1=10kΩ (superior) e R2=5kΩ (inferior)
// Fator = (10+5)/5 = 3.0
// =============================================
const float R1_divisor   = 10.0;  // kΩ
const float R2_divisor   = 5.0;   // kΩ
const float fatorDivisor = (R1_divisor + R2_divisor) / R2_divisor; // = 3.0

// =============================================
// CONSTANTES DO SHUNT DE CORRENTE
// Rshunt=0.1Ω e ganho do op-amp = 15k/1k = 15
// =============================================
const float Rshunt     = 0.1;   // Ω
const float ganhoShunt = 15.0;  // ganho do op-amp
//=============================================
// CONSTANTES DO SENSOR DE TEMPERATURA (NTC)
// =============================================
const float BETA      = 3950.0;
const float R0        = 10000.0;
const float T0        = 298.15;
const float RES_FIXO  = 10000.0;

// =============================================
// REFERÊNCIA DO ADC
// =============================================
const float V_REF   = 3.3;
const int   ADC_RES = 4095;

// =============================================
// LIMITES DE SEGURANÇA
// =============================================
const float VOLT_MIN = 6.4;   // Subtensão crítica
const float VOLT_MAX = 8.4;   // Sobretensão
const float TEMP_MAX = 50.0;  // Limite térmico

// =============================================
// VARIÁVEIS GLOBAIS
// =============================================
bool sistemaSeguro           = true;
float energiaAcumulada_Wh    = 0.0;
unsigned long tempoAnterior  = 0;
unsigned long ultimoEnvio    = 0;

// =============================================
// PROTÓTIPOS
// =============================================
float lerTensao();
float lerCorrente();
float lerTemperatura();
void  verificarSeguranca(float v, float t);
void  enviarDados(float v, float i, float p, float e);
void  moverFrente(int velocidade);
void  pararMotores();

// =============================================
// SETUP
// =============================================
void setup() {
  Serial.begin(115200);

  // Pinos de motor
  pinMode(pinoMotorA1, OUTPUT);
  pinMode(pinoMotorA2, OUTPUT);
  pinMode(pinoMotorB1, OUTPUT);
  pinMode(pinoMotorB2, OUTPUT);
  pinMode(pinoEna,     OUTPUT);
  pinMode(pinoEnb,     OUTPUT);

  // Pinos de interface
  pinMode(pinoLedR,   OUTPUT);
  pinMode(pinoLedG,   OUTPUT);
  pinMode(pinoBuzzer, OUTPUT);

  pararMotores();

  // LED amarelo durante conexão WiFi
  digitalWrite(pinoLedR, HIGH);
  digitalWrite(pinoLedG, HIGH);

  // Conecta WiFi
  Serial.print("Conectando ao WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado! IP: " + WiFi.localIP().toString());

  // LED verde — sistema pronto
  digitalWrite(pinoLedR, LOW);
  digitalWrite(pinoLedG, HIGH);

  tempoAnterior = millis();
}

// =============================================
// LOOP PRINCIPAL
// =============================================
void loop() {
  // --- 1. LEITURA DOS SENSORES ---
  float tensao      = lerTensao();
  float corrente    = lerCorrente();
  float temperatura = lerTemperatura();

  // --- 2. CÁLCULO DE POTÊNCIA E ENERGIA ---
  float potencia = tensao * corrente;

  unsigned long agora      = millis();
  float deltaT_horas       = (agora - tempoAnterior) / 3600000.0;
  energiaAcumulada_Wh     += potencia * deltaT_horas;
  tempoAnterior            = agora;

  // --- 3. VERIFICAÇÃO DE SEGURANÇA ---
  verificarSeguranca(tensao, temperatura);

  // --- 4. CONTROLE DOS MOTORES ---
  if (sistemaSeguro) {
    moverFrente(150);
    digitalWrite(pinoLedG, HIGH);
    digitalWrite(pinoLedR, LOW);
  } else {
    pararMotores();
  }

  // --- 5. ENVIO VIA WIFI (a cada 100ms) ---
  if (agora - ultimoEnvio >= 100) {
    enviarDados(tensao, corrente, potencia, energiaAcumulada_Wh);
    ultimoEnvio = agora;
  }

  // --- 6. DEBUG SERIAL ---
  Serial.printf(
    "V=%.3fV | I=%.4fA | P=%.3fW | E=%.6fWh | T=%.1fC | %s\n",
    tensao, corrente, potencia, energiaAcumulada_Wh,
    temperatura, sistemaSeguro ? "OK" : "ERRO"
  );

  delay(100);
}

// =============================================
// FUNÇÕES DE SENSORES
// =============================================

float lerTensao() {
  int raw = analogRead(pinoVBat);
  float vADC = raw * (V_REF / ADC_RES);
  return vADC * fatorDivisor;  // Recupera tensão real da bateria
}

float lerCorrente() {
  int raw = analogRead(pinoCorrente);
  float vADC = raw * (V_REF / ADC_RES);
  return vADC / (Rshunt * ganhoShunt);  // Corrente real em Ampères
}

float lerTemperatura() {
  int raw = analogRead(pinoTemp);

  // Detecção de rompimento de chicote
  if (raw > 4000 || raw < 100) return 999.0;

  float vOut = raw * (V_REF / ADC_RES);
  float rNTC = RES_FIXO * ((V_REF / vOut) - 1.0);

  // Equação Beta
  float tempK = 1.0 / ((log(rNTC / R0) / BETA) + (1.0 / T0));
  return tempK - 273.15;
}

// =============================================
// SEGURANÇA
// =============================================

void verificarSeguranca(float v, float t) {
  bool erroVolt = (v < VOLT_MIN || v > VOLT_MAX);
  bool erroTemp = (t > TEMP_MAX || t == 999.0);

  if (erroVolt || erroTemp) {
    sistemaSeguro = false;
    digitalWrite(pinoLedG,  LOW);
    digitalWrite(pinoLedR,  HIGH);  // Vermelho: erro
    digitalWrite(pinoBuzzer, HIGH);

    // Log do tipo de erro
    if (erroVolt) Serial.println("ERRO: Tensão fora dos limites!");
    if (erroTemp) Serial.println("ERRO: Temperatura fora dos limites ou chicote rompido!");
  } else {
    sistemaSeguro = true;
    digitalWrite(pinoLedR,   LOW);
    digitalWrite(pinoLedG,   HIGH); // Verde: seguro
    digitalWrite(pinoBuzzer, LOW);
  }
}

// =============================================
// ENVIO DE DADOS VIA WIFI
// =============================================

void enviarDados(float v, float i, float p, float e) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado! Reconectando...");
    WiFi.reconnect();
    return;
  }

  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"tensao\":"   + String(v, 4) + ",";
  payload += "\"corrente\":" + String(i, 4) + ",";
  payload += "\"potencia\":" + String(p, 4) + ",";
  payload += "\"energia\":"  + String(e, 6);
  payload += "}";

  int httpCode = http.POST(payload);
  if (httpCode < 0) {
    Serial.println("Erro no envio HTTP: " + String(httpCode));
  }
  http.end();
}

// =============================================
// CONTROLE DE MOTORES
// =============================================

void moverFrente(int velocidade) {
  digitalWrite(pinoMotorA1, HIGH);
  digitalWrite(pinoMotorA2, LOW);
  digitalWrite(pinoMotorB1, HIGH);
  digitalWrite(pinoMotorB2, LOW);
  analogWrite(pinoEna, velocidade);
  analogWrite(pinoEnb, velocidade);
}

void pararMotores() {
  digitalWrite(pinoMotorA1, LOW);
  digitalWrite(pinoMotorA2, LOW);
  digitalWrite(pinoMotorB1, LOW);
  digitalWrite(pinoMotorB2, LOW);
  analogWrite(pinoEna, 0);
  analogWrite(pinoEnb, 0);
}
