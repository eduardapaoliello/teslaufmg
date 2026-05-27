#include <Arduino.h>

// =============================================
// PARÂMETROS E CONSTANTES DE SEGURANÇA
// =============================================
const float VOLT_MIN = 6.4;  // Tensão mínima permitida (ex: bateria descarregada)
const float VOLT_MAX = 8.4;  // Tensão máxima permitida (ex: sobrecarga)
const float TEMP_MAX = 50.0; // Temperatura máxima permitida em °C

// Pinos de alerta
const int pinoLedR   = 25;   // LED Vermelho (Erro)
const int pinoLedG   = 22;   // LED Verde (OK)
const int pinoBuzzer = 4;    // Buzzer de alarme

// Variável de controle do estado do sistema
bool sistemaSeguro = true;

// Protótipos das funções
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
  // SIMULAÇÃO DE TESTE (Altere estes valores para testar os gatilhos)
  // ======================================================================
  float tensaoSimulada = 7.4;      // Dentro do range (6.4V a 8.4V)
  float temperaturaSimulada = 35.5; // Dentro do range (Abaixo de 50°C)
  // ======================================================================

  // Executa a verificação
  verificarSeguranca(tensaoSimulada, temperaturaSimulada);

  // Lógica de ação baseada no estado de segurança
  if (sistemaSeguro) {
    // Sistema operando normalmente
    digitalWrite(pinoLedG, HIGH);
    digitalWrite(pinoLedR, LOW);
    noTone(pinoBuzzer); // Garante que o alarme está desligado
    
    Serial.printf("SISTEMA OK | V: %.2fV | T: %.1f°C\n", tensaoSimulada, temperaturaSimulada);
  } else {
    // Bloqueio de emergência ativado
    para_tudo();
    digitalWrite(pinoLedG, LOW);
    digitalWrite(pinoLedR, HIGH);
    
    // Liga o alarme sonoro (Frequência de 1000Hz)
    tone(pinoBuzzer, 1000); 
    
    Serial.printf("!!! ALERTA DE SEGURANÇA !!! V: %.2fV | T: %.1f°C\n", tensaoSimulada, temperaturaSimulada);
  }

  delay(1000); // Executa a checagem a cada 1 segundo
}

// =============================================
// FUNÇÃO DE VERIFICAÇÃO (ISOLADA)
// =============================================
void verificarSeguranca(float v, float t) {
  // Retorna verdadeiro se a tensão estiver fora dos limites
  bool erroVolt = (v < VOLT_MIN || v > VOLT_MAX);
  
  // Retorna verdadeiro se a temperatura passar do limite ou se o sensor falhar (999.0)
  bool erroTemp = (t > TEMP_MAX || t == 999.0);

  // Se houver qualquer erro, desarma o sistema seguro
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
    // Aqui no código original você limpa as variáveis dos motores.
    // Para o teste isolado, esta função serve para garantir o desligamento.
    noTone(pinoBuzzer); 
    
    // Exemplo: Colocar pinos de ponte H em LOW aqui para cortar os motores fisicamente
    // digitalWrite(pinoMotorA1, LOW); 
    // ...
}
