#include <math.h>
#include <Arduino.h>

// =============================================
// MAPEAMENTO DE PINOS (ESP32)
// =============================================
const int pinoTemp = 33;

// =============================================
// CONSTANTES DO TERMISTOR (Steinhart-Hart)
// =============================================
const float A = 0.001129241;
const float B = 0.0002341077;
const float C = 0.00000008775468;
const float RES_FIXO = 10000.0;

const float V_REF   = 3.3;
const int   ADC_RES = 4095;

float lerTemperatura();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("--- Teste de Leitura de Temperatura ---");
}

void loop() {
  float temperatura = lerTemperatura();
  
  Serial.print("Temperatura: ");
  if (temperatura == 999.0) {
    Serial.println("ERRO (Sensor desconectado ou em curto)");
  } else {
    Serial.print(temperatura, 1);
    Serial.println(" °C");
  }

  delay(1000);
}

// =============================================
// FUNÇÃO DE LEITURA ISOLADA
// =============================================
float lerTemperatura() {
  int raw = analogRead(pinoTemp);
  
  if (raw > 4000 || raw < 100) return 999.0;
  
  float rNTC = RES_FIXO * (((float)ADC_RES / raw) - 1.0);
  
  float logR = log(rNTC);
  float umSobreT = A + (B * logR) + (C * logR * logR * logR);
  float tempK = 1.0 / umSobreT;
  
  return tempK - 273.15;
}
