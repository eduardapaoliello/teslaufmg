#include <math.h>
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

// =============================================
// CONFIGURAÇÕES DE REDE
// =============================================
const char* ssid      = "SUA_REDE_WIFI";
const char* password  = "SUA_SENHA_WIFI";
const char* serverURL = "http://192.168.68.105:5000/dados"; 

// =============================================
// MAPEAMENTO DE PINOS (ESP32)
// =============================================
const int pinoVBat     = 34;   
const int pinoCorrente = 35;   
const int pinoTemp     = 32;   

const int pinoMotorA1  = 12;
const int pinoMotorA2  = 13;
const int pinoMotorB1  = 14;
const int pinoMotorB2  = 27;
const int pinoEna      = 33;
const int pinoEnb      = 26;

const int pinoLedR     = 25; 
const int pinoLedG     = 22;
const int pinoBuzzer   = 4;

const int pinoBotao    = 7;
const int pinoTeste    = 2;
const int pinoSD_CS    = 5;

// =============================================
// CONSTANTES E VARIÁVEIS TÉCNICAS
// =============================================
const float R1_divisor   = 10.0;  
const float R2_divisor   = 5.0;  
const float fatorDivisor = (R1_divisor + R2_divisor) / R2_divisor; 

const float Rshunt       = 0.1;   
const float ganhoShunt   = 15.0;  

const float A = 0.001129241;
const float B = 0.0002341077;
const float C = 0.00000008775468;
const float RES_FIXO     = 10000.0;

const float V_REF   = 3.3;
const int   ADC_RES = 4095;

const float VOLT_MIN = 6.4;  
const float VOLT_MAX = 8.4;  
const float TEMP_MAX = 50.0; 

bool sistemaSeguro           = true;
bool rodando                 = false; 
unsigned long tempo_inicial  = 0;
unsigned long agora          = 0;
int etapa                    = 0; 

float energiaAcumulada_Wh    = 0.0;
unsigned long tempoAnterior  = 0;
unsigned long ultimoEnvio    = 0;
unsigned long ultimoLogSD    = 0;

const char* nomeArquivo = "/log_energia.csv";

// =============================================
// PROTÓTIPOS
// =============================================
float lerTensao();
float lerCorrente();
float lerTemperatura();
void  verificarSeguranca(float v, float t);
void  enviarDados(float v, float i, float p, float e);
void  salvarNoSD(float v, float i, float p, float e, float t);
void  gerencia_motor();
void  para_tudo();
void  configurarDirecaoFrente();
void  pararMotores();

// =============================================
// SETUP
// =============================================
void setup() {
  Serial.begin(115200);

  pinMode(pinoMotorA1, OUTPUT);
  pinMode(pinoMotorA2, OUTPUT);
  pinMode(pinoMotorB1, OUTPUT);
  pinMode(pinoMotorB2, OUTPUT);
  pinMode(pinoEna,      OUTPUT);
  pinMode(pinoEnb,      OUTPUT);

  pinMode(pinoLedR,   OUTPUT);
  pinMode(pinoLedG,   OUTPUT);
  pinMode(pinoBuzzer, OUTPUT);
  
  pinMode(pinoBotao, INPUT_PULLUP);
  pinMode(pinoTeste, INPUT_PULLUP);

  pararMotores();

  Serial.print("Iniciando cartao SD...");
  if (!SD.begin(pinoSD_CS)) {
    Serial.println(" FALHA! Verifique o cartao.");
  } else {
    Serial.println(" OK!");
    File arquivo = SD.open(nomeArquivo, FILE_APPEND);
    if (arquivo) {
      if (arquivo.size() == 0) {
        arquivo.println("Timestamp_ms;Tensao_V;Corrente_A;Potencia_W;Energia_Wh;Temp_C");
      }
      arquivo.close();
    }
  }

  digitalWrite(pinoLedR, HIGH);
  digitalWrite(pinoLedG, HIGH);

  Serial.print("Conectando ao WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado! IP: " + WiFi.localIP().toString());

  digitalWrite(pinoLedR, LOW);
  digitalWrite(pinoLedG, HIGH);

  tempoAnterior = millis();
}

// =============================================
// LOOP PRINCIPAL
// =============================================
void loop() {
  agora = millis(); 

  float tensao      = lerTensao();
  float corrente    = lerCorrente();
  float temperatura = lerTemperatura();
  float potencia    = tensao * corrente;

  float deltaT_horas       = (agora - tempoAnterior) / 3600000.0;
  energiaAcumulada_Wh     += potencia * deltaT_horas;
  tempoAnterior            = agora;

  verificarSeguranca(tensao, temperatura);

  if (sistemaSeguro) {
    if (digitalRead(pinoTeste) == LOW) {
        para_tudo();
    }
    
    gerencia_motor();
    digitalWrite(pinoLedG, rodando);
    digitalWrite(pinoLedR, !rodando); 
  } else {
    
    para_tudo();
    digitalWrite(pinoLedG, LOW);
    digitalWrite(pinoLedR, HIGH);
    digitalWrite(pinoBuzzer, HIGH);
  }

  if (agora - ultimoEnvio >= 100) {
    enviarDados(tensao, corrente, potencia, energiaAcumulada_Wh);
    ultimoEnvio = agora;
  }

  if (agora - ultimoLogSD >= 500) {
    salvarNoSD(tensao, corrente, potencia, energiaAcumulada_Wh, temperatura);
    ultimoLogSD = agora;
  }

  Serial.printf(
    "V=%.3fV | I=%.4fA | P=%.3fW | E=%.6fWh | T=%.1fC | Rotina=%s | Seg=%s\n",
    tensao, corrente, potencia, energiaAcumulada_Wh,
    temperatura, rodando ? "ATIVO" : "PARADO", sistemaSeguro ? "OK" : "ERRO"
  );

  delay(10);
}

// =============================================
// FUNÇÕES DE SENSORES E LOG
// =============================================

float lerTensao() {
  int raw = analogRead(pinoVBat);
  float vADC = raw * (V_REF / ADC_RES);
  return vADC * fatorDivisor;
}

float lerCorrente() {
  int raw = analogRead(pinoCorrente);
  float vADC = raw * (V_REF / ADC_RES);
  return vADC / (Rshunt * ganhoShunt);
}

float lerTemperatura() {
  int raw = analogRead(pinoTemp);
  if (raw > 4000 || raw < 100) return 999.0;
  float vOut = raw * (V_REF / ADC_RES);
  float rNTC = RES_FIXO * ((V_REF / vOut) - 1.0);
  float logR = log(rNTC);
  float umSobreT = A + (B * logR) + (C * logR * logR * logR);
  float tempK = 1.0 / umSobreT;
  return tempK - 273.15;
}

void salvarNoSD(float v, float i, float p, float e, float t) {
  File arquivo = SD.open(nomeArquivo, FILE_APPEND);
  if (arquivo) {
    arquivo.print(millis());
    arquivo.print(";");
    arquivo.print(v, 3);
    arquivo.print(";");
    arquivo.print(i, 4);
    arquivo.print(";");
    arquivo.print(p, 3);
    arquivo.print(";");
    arquivo.print(e, 6);
    arquivo.print(";");
    arquivo.println(t, 1);
    arquivo.close();
  }
}

// =============================================
// SEGURANÇA
// =============================================

void verificarSeguranca(float v, float t) {
  bool erroVolt = (v < VOLT_MIN || v > VOLT_MAX);
  bool erroTemp = (t > TEMP_MAX || t == 999.0);

  if (erroVolt || erroTemp) {
    sistemaSeguro = false;
  } else {
    sistemaSeguro = true;
  }
}

// =============================================
// MÁQUINA DE ESTADOS - ROTINA DOS MOTORES (CÓDIGO 1)
// =============================================

void para_tudo() {
    etapa = 0;
    rodando = false;
    noTone(pinoBuzzer);
    pararMotores();
}

void gerencia_motor() {
    if (digitalRead(pinoBotao) == LOW && etapa == 0) {
        tempo_inicial = agora; 
        etapa = 1;
        tone(pinoBuzzer, 440); 
        rodando = true;
    }

    if (etapa == 1 && agora - tempo_inicial > 3000) {
        noTone(pinoBuzzer);
        configurarDirecaoFrente();
        analogWrite(pinoEna, 255);
        analogWrite(pinoEnb, 255); 
        etapa++;
    }

    if (etapa == 2 && agora - tempo_inicial > 5000) {
        analogWrite(pinoEnb, 128); 
        etapa++; 
    }

    if (etapa == 3 && agora - tempo_inicial > 7000) {
        analogWrite(pinoEna, 128);
        analogWrite(pinoEnb, 255); 
        etapa++;
    }

    if (etapa == 4 && agora - tempo_inicial > 9000) {
        pararMotores();
        etapa = 0;
        rodando = false;
    }
}

// =============================================
// ENVIO DE DADOS VIA WIFI
// =============================================

void enviarDados(float v, float i, float p, float e) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"tensao\":" + String(v, 4) + 
                   ",\"corrente\":" + String(i, 4) + 
                   ",\"potencia\":" + String(p, 4) + 
                   ",\"energia\":" + String(e, 6) + "}";

  http.POST(payload);
  http.end();
}

// =============================================
// CONTROLE DE MOTORES (DRIVER PONTE H)
// =============================================

void configurarDirecaoFrente() {
  digitalWrite(pinoMotorA1, HIGH);
  digitalWrite(pinoMotorA2, LOW);
  digitalWrite(pinoMotorB1, HIGH);
  digitalWrite(pinoMotorB2, LOW);
}

void pararMotores() {
  digitalWrite(pinoMotorA1, LOW);
  digitalWrite(pinoMotorA2, LOW);
  digitalWrite(pinoMotorB1, LOW);
  digitalWrite(pinoMotorB2, LOW);
  analogWrite(pinoEna, 0);
  analogWrite(pinoEnb, 0);
}
