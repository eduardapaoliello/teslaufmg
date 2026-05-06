#include <math.h>
#include <Arduino.h>

// Protótipos das funções
float lerTensao();
float lerTemperatura();
void verificarSeguranca(float v, float t);
void moverFrente(int velocidade);
void pararMotores();

// --- MAPEAMENTO DE PINOS (ESP32) ---
// Sensores (ADC)
const int pinoVBat = 34;    // Monitoramento de Tensão (via LM324)
const int pinoTemp = 35;    // Monitoramento de Temperatura (via LM324)

// Controle Ponte H L298N
const int pinoMotorA1 = 12; // IN1
const int pinoMotorA2 = 13; // IN2
const int pinoMotorB1 = 14; // IN3
const int pinoMotorB2 = 27; // IN4
const int pinoEna = 33;     // PWM Motor A
const int pinoEnb = 32;     // PWM Motor B

// Interface de Saída
const int pinoLedR = 25;    // LED RGB Vermelho
const int pinoLedG = 26;    // LED RGB Verde
const int pinoBuzzer = 19;

// --- CONSTANTES TÉCNICAS ---
const float BETA = 3950.0;       // Coeficiente Beta
const float R0 = 10000.0;        // 10k Ohms a 25°C
const float T0 = 298.15;         // 25°C em Kelvin
const float RES_FIXO = 10000.0;  // Resistor fixo do divisor
const float V_REF = 3.3;         // Tensão de operação ESP32
const int ADC_RES = 4095;        // Resolução 12 bits

// --- LIMITES DE SEGURANÇA ---
const float VOLT_MIN = 6.4;      // Bateria Li-ion crítica
const float VOLT_MAX = 8.4;      // Sobretensão carga total
const float TEMP_MAX = 50.0;     // Limite térmico bateria

// --- VARIÁVEIS GLOBAIS ---
bool sistemaSeguro = true;

void setup() {
  Serial.begin(115200);
  
  // Configuração dos pinos
  pinMode(pinoMotorA1, OUTPUT);
  pinMode(pinoMotorA2, OUTPUT);
  pinMode(pinoMotorB1, OUTPUT);
  pinMode(pinoMotorB2, OUTPUT);
  pinMode(pinoEna, OUTPUT);
  pinMode(pinoEnb, OUTPUT);
  
  pinMode(pinoLedR, OUTPUT);
  pinMode(pinoLedG, OUTPUT);
  pinMode(pinoBuzzer, OUTPUT);

  // Inicializa motores parados
  pararMotores();
}

void loop() {
  float tensao = lerTensao();
  float temperatura = lerTemperatura();

  verificarSeguranca(tensao, temperatura);

  if (sistemaSeguro) {
    // Exemplo: Mover para frente se o sistema estiver OK
    moverFrente(150); 
  } else {
    pararMotores();
  }

  // Debug no Monitor Serial
  Serial.printf("Tensão: %.2fV | Temp: %.2fC | Status: %s\n", 
                tensao, temperatura, sistemaSeguro ? "OK" : "ERRO");
  
  delay(500);
}

// --- FUNÇÕES DE SENSORES ---

float lerTensao() {
  int raw = analogRead(pinoVBat);
  // Compensação do divisor 15k/10k (Vin = Vout * 2.5)
  float vOut = (raw * V_REF) / ADC_RES;
  return vOut * 2.5;
}

float lerTemperatura() {
  int raw = analogRead(pinoTemp);

  // Detecção de Rompimento de Chicote
  if (raw > 4000 || raw < 100) return 999.0;

  float vOut = (raw * V_REF) / ADC_RES;
  // Cálculo da resistência do NTC
  float rNTC = RES_FIXO * ((V_REF / vOut) - 1.0);

  // Implementação da Equação Beta
  float tempK;
  tempK = log(rNTC / R0);
  tempK /= BETA;
  tempK += 1.0 / T0;
  tempK = 1.0 / tempK;

  return tempK - 273.15; // Retorna Celsius
}

// --- LÓGICA DE SEGURANÇA E ERRO ---

void verificarSeguranca(float v, float t) {
  // Verificação de faixas e chicote
  bool erroVolt = (v < VOLT_MIN || v > VOLT_MAX);
  bool erroTemp = (t > TEMP_MAX || t == 999.0);

  if (erroVolt || erroTemp) {
    sistemaSeguro = false;
    digitalWrite(pinoLedG, LOW);
    digitalWrite(pinoLedR, HIGH); // Vermelho: Erro
    digitalWrite(pinoBuzzer, HIGH);
  } else {
    sistemaSeguro = true;
    digitalWrite(pinoLedR, LOW);
    digitalWrite(pinoLedG, HIGH); // Verde: Seguro
    digitalWrite(pinoBuzzer, LOW);
  }
}

// --- CONTROLE DE MOVIMENTO ---

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