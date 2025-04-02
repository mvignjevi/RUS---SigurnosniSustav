//Definiranje komponenti i pinova

const int button1 = 2;  // Pin za gumb 1
const int button2 = 3;  // Pin za gumb 2  
const int led1 = 9;     // Pin za LED1 (koji treperi)
const int led2 = 10;    // Pin za LED2 (koji se pali na pritisak tipkala)
const int potPin = A0;  // Pin za potenciometar (simulacija senzora)


//definiranje volatile varijabli za kontrolu toka van ISR-a
volatile bool flag1 = false;
volatile bool flag2 = false;

// ISR za gumb 1
void ISR_Button1() {
    flag1 = true; //ako je zeleni gumb pritisnut postavlja se flag1 na true
}

// ISR za gumb 2
void ISR_Button2() {  ////ako je plavi gumb pritisnut postavlja se flag1 na true
    flag2 = true;
}

// ISR za Timer1 (trepereci LED1)
ISR(TIMER1_COMPA_vect) {
    digitalWrite(led1, !digitalRead(led1));
}

void setup() {
    pinMode(button1, INPUT_PULLUP);
    pinMode(button2, INPUT_PULLUP);
    pinMode(led1, OUTPUT);
    pinMode(led2, OUTPUT);
    pinMode(potPin, INPUT);

    Serial.begin(9600);
    
    // Postavljanje vanjskih prekida
    attachInterrupt(digitalPinToInterrupt(button1), ISR_Button1, FALLING);
    attachInterrupt(digitalPinToInterrupt(button2), ISR_Button2, FALLING);

    // Postavljanje Timer1 prekida
    noInterrupts(); 
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    OCR1A = 15624; // 1Hz (16MHz / (1024 * 1Hz) - 1)
    TCCR1B |= (1 << WGM12); // CTC mod
    TCCR1B |= (1 << CS12) | (1 << CS10);
    TIMSK1 |= (1 << OCIE1A);
    interrupts(); 
}

void loop() {
    static unsigned long lastPrint = 0;

    // Obrada prekida za gumbove 1 i 2
    if (flag1) {
        Serial.println("Prekid od tipkala 1");
        digitalWrite(led2, HIGH);
        delay(500); //LED2(plava) ostaje upaljena 0,5s
        digitalWrite(led2, LOW);
        flag1 = false;
    }

    if (flag2) {
        Serial.println("Prekid od tipkala 2");
        digitalWrite(led2, HIGH);
        delay(2000); //LED2(plava) ostaje upaljena 2s
        digitalWrite(led2, LOW);
        flag2 = false;
    }

    // Simulacija očitavanja sa senzora
    int potValue = analogRead(potPin);
    
    if (millis() - lastPrint > 5000) {  // Očitavanje svakih 5 sekundi
        Serial.print("Simulirana vrijednost senzora (potenciometar): ");
        Serial.println(potValue);
        lastPrint = millis();
    }
}
