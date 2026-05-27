#define EN_A 6
#define EN_B 5
#define botao 7 
#define buzzer 9
#define LED_rodando 13
#define LED_parado 12
#define teste 2

bool rodando = false; 
unsigned long tempo_inicial, agora;
int etapa = 0; 

void para_tudo(){
    etapa = 0;
    rodando = false;
    noTone(buzzer);
    digitalWrite(EN_A, LOW);
    digitalWrite(EN_B, LOW);
}
void gerencia_motor() {
    agora = millis(); 

    if (digitalRead(botao) == LOW && etapa == 0) {
        tempo_inicial = agora; 
        etapa = 1;
        tone(buzzer, 440); 
        rodando = true;
    }

    if (etapa == 1 && agora - tempo_inicial > 3000) {
        noTone(buzzer);
        digitalWrite(EN_A, HIGH);
        digitalWrite(EN_B, HIGH);
        etapa++;
    }

    if (etapa == 2 && agora - tempo_inicial > 5000) {
        analogWrite(EN_B, 128); 
        etapa++; 
    }

    if (etapa == 3 && agora - tempo_inicial > 7000) {
        analogWrite(EN_A, 128);
        analogWrite(EN_B, 255); 
        etapa++;
    }

    if (etapa == 4 && agora - tempo_inicial > 9000) {
        digitalWrite(EN_A, LOW);
        digitalWrite(EN_B, LOW);
        etapa = 0;
        rodando = false;
    }
}

void setup() {
    pinMode(EN_A, OUTPUT);
    pinMode(EN_B, OUTPUT);
    pinMode(buzzer, OUTPUT);
    pinMode(botao, INPUT_PULLUP);
    pinMode(LED_rodando, OUTPUT);
    pinMode(LED_parado, OUTPUT);
    pinMode(teste, INPUT_PULLUP);
}

void loop() {
   
    digitalWrite(LED_rodando, rodando);
    digitalWrite(LED_parado, !rodando); 
    if(digitalRead(teste) == LOW) para_tudo();
    gerencia_motor();
    
}
