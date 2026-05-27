#include <Arduino.h>

// =============================================
// PARÂMETROS E CONSTANTES DE SEGURANÇA
// =============================================
const float VOLT_MIN = 6.4;
const float VOLT_MAX = 8.4;
const float TEMP_MAX = 50.0;

const int pinoLedR   = 25;
const int pinoLedG   = 22;
const int pinoBuzzer = 4;

bool sistemaSeguro = true;

void verificarSeguranca(float v, float t);
void para_tudo();

void setup() {
  Serial.begin(115200);
  
  pinMode(pinoLedR, OUTPUT);
  pinMode(pinoLedG, OUTPUT);
  pinMode(pinoBuzzer, OUTPUT);
  
  Serial.println("--- Teste do Módulo de Segurança ---");
}

void loop() {
  // ======================================================================
  // SIMULAÇÃO DE TESTE
  // ======================================================================
  float tensaoSimulada = 7.4;
  float temperaturaSimulada = 35.5;
  // ======================================================================

  verificarSeguranca(tensaoSimulada, temperaturaSimulada);

  if (sistemaSeguro) {
    digitalWrite(pinoLedG, HIGH);
    digitalWrite(pinoLedR, LOW);
    noTone(pinoBuzzer);
    
    Serial.printf("SISTEMA OK | V: %.2fV | T: %.1f°C\n", tensaoSimulada, temperaturaSimulada);
  } else {
    para_tudo();
    digitalWrite(pinoLedG, LOW);
    digitalWrite(pinoLedR, HIGH);
    
    tone(pinoBuzzer, 1000); 
    
    Serial.printf("!!! ALERTA DE SEGURANÇA !!! V: %.2fV | T: %.1f°C\n", tensaoSimulada, temperaturaSimulada);
  }

  delay(100);
}

// =============================================
// FUNÇÃO DE VERIFICAÇÃO (ISOLADA)
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
// FUNÇÃO DE DESARMAMENTO (ISOLADA)
// =============================================
void para_tudo() {
    noTone(pinoBuzzer); 
}
